#pragma once
#include "Wavetables.h"
#include <cstdint>

namespace xs
{
inline float fastTanh (float x) noexcept
{
    x = juce::jlimit (-4.5f, 4.5f, x);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float msToCoef (float ms, float sr, float target = 1.0e-3f) noexcept
{
    const float n = juce::jmax (1.0f, ms * 0.001f * sr);
    return std::exp (std::log (target) / n);
}

// Click-free ADSR: linear attack from the current level, exponential decay/release.
struct Envelope
{
    enum State { Idle, Attack, Decay, Sustain, Release };
    State state = Idle;
    float level = 0.0f, attackInc = 1.0f, decayCoef = 0.0f, releaseCoef = 0.0f, sustain = 1.0f;

    void set (float aMs, float dMs, float s, float rMs, float sr) noexcept
    {
        attackInc = 1.0f / juce::jmax (1.0f, juce::jmax (1.0f, aMs) * 0.001f * sr); // >= 1 ms
        decayCoef = msToCoef (dMs, sr);
        releaseCoef = msToCoef (juce::jmax (4.0f, rMs), sr, 1.0e-4f);                  // >= 4 ms
        sustain = s;
    }
    void noteOn() noexcept   { state = Attack; }
    void noteOff() noexcept  { if (state != Idle) state = Release; }
    void reset() noexcept    { state = Idle; level = 0.0f; }
    bool active() const noexcept { return state != Idle; }

    inline float next() noexcept
    {
        switch (state)
        {
            case Attack:
                level += attackInc;
                if (level >= 1.0f) { level = 1.0f; state = Decay; }
                break;
            case Decay:
                level = sustain + (level - sustain) * decayCoef;
                if (std::abs (level - sustain) < 1.0e-4f) { level = sustain; state = Sustain; }
                break;
            case Sustain: level = sustain; break;
            case Release:
                level *= releaseCoef;
                if (level < 1.0e-5f) { level = 0.0f; state = Idle; }
                break;
            case Idle: default: break;
        }
        return level;
    }
};

// Cytomic/Simper trapezoidal state-variable filter.
struct SVF
{
    float ic1 = 0.0f, ic2 = 0.0f;
    void reset() noexcept { ic1 = ic2 = 0.0f; }
    // mode: 0 LP, 1 HP, 2 BP
    inline float process (float x, float g, float k, int mode) noexcept
    {
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + g * a1 * v3;
        const float v2 = ic2 + g * v1;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        if (mode == 0) return v2;
        if (mode == 1) return x - k * v1 - v2;
        return v1 * k;  // unity-peak band-pass
    }
};

// Per-chunk control values shared by all voices (already smoothed by the engine).
struct VoiceCtrl
{
    static constexpr int kMax = 32;
    int n = 0;
    float sr = 48000.0f;
    // per-sample smoothed
    float waveA[kMax], waveB[kMax], levelA[kMax], levelB[kMax], subLevel[kMax], noiseLevel[kMax];
    float cutoff[kMax], res[kMax], drive[kMax], lfo[kMax];
    // per-chunk
    bool aOn = true, bOn = false;
    int aUni = 1, bUni = 1;
    float aRatio = 1.0f, bRatio = 1.0f, aDetune = 0.0f, bDetune = 0.0f, aWidth = 0.0f, bWidth = 0.0f;
    float subRatio = 0.5f, subShape = 0.0f, noiseColour = 0.5f;
    int filterType = 1;
    float fltEnv = 0.0f, keyTrack = 0.0f;
    float penvAmt = 0.0f, penvCoef = 0.99f;
    int lfoDest = 1;
    float lfoAmt = 0.0f;
    float glideCoef = 0.0f;           // 0 = jump
    float bendSemis = 0.0f;
    float velSens = 0.5f;
    float aEnv[4] { 2, 400, 0.8f, 150 }, mEnv[4] { 2, 400, 0.3f, 300 };
};

class Voice
{
public:
    static constexpr int kMaxUni = 7;

    void prepare (float sampleRate) noexcept { sr = sampleRate; resetState(); }

    bool isActive() const noexcept { return ampEnv.active(); }
    bool isReleasing() const noexcept { return ampEnv.state == Envelope::Release; }
    int note = -1;
    uint64_t age = 0;

    // fresh = voice was silent: reset oscillators/filters so the transient is consistent.
    void start (int midiNote, float velocity, float fromPitch, bool glideIn, bool retrigger, const VoiceCtrl& c, uint32_t seed)
    {
        const bool fresh = ! ampEnv.active();
        note = midiNote;
        targetPitch = (float) midiNote;
        pitch = (glideIn && fromPitch >= 0.0f) ? fromPitch : (float) midiNote;
        vel = velocity;
        if (fresh)
        {
            rng = seed | 1u;
            for (int i = 0; i < kMaxUni; ++i)
            {
                phA[i] = i == 0 ? 0.0f : rand01();
                phB[i] = i == 0 ? 0.0f : rand01();
            }
            phSub = 0.0f;
            for (auto& f : filt) f.reset();
            noiseLp = 0.0f;
        }
        if (retrigger || fresh)
        {
            updateEnvs (c);
            ampEnv.noteOn();
            modEnv.noteOn();
            penv = 1.0f;
        }
    }

    void legatoTo (int midiNote) noexcept { note = midiNote; targetPitch = (float) midiNote; }
    void release() noexcept { ampEnv.noteOff(); modEnv.noteOff(); }
    void kill() noexcept { ampEnv.reset(); modEnv.reset(); note = -1; }
    float currentPitch() const noexcept { return pitch; }

    void updateEnvs (const VoiceCtrl& c) noexcept
    {
        ampEnv.set (c.aEnv[0], c.aEnv[1], c.aEnv[2], c.aEnv[3], sr);
        modEnv.set (c.mEnv[0], c.mEnv[1], c.mEnv[2], c.mEnv[3], sr);
    }

    void render (const VoiceCtrl& c, float* outL, float* outR) noexcept
    {
        const int n = c.n;
        const auto& bank = WavetableBank::get();
        updateEnvs (c);

        // pass 1: pitch per sample (glide, pitch env, bend, vibrato)
        float freq[VoiceCtrl::kMax];
        float maxFreq = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            if (c.glideCoef > 0.0f) pitch = targetPitch + (pitch - targetPitch) * c.glideCoef;
            else pitch = targetPitch;
            penv *= c.penvCoef;
            float p = pitch + c.penvAmt * penv + c.bendSemis;
            if (c.lfoDest == 0) p += c.lfo[i] * c.lfoAmt * c.lfoAmt * 12.0f;
            freq[i] = 440.0f * std::exp2 ((p - 69.0f) * (1.0f / 12.0f));
            maxFreq = juce::jmax (maxFreq, freq[i]);
        }

        // unison tables for this chunk
        float uniRA[kMaxUni], panLA[kMaxUni], panRA[kMaxUni], uniRB[kMaxUni], panLB[kMaxUni], panRB[kMaxUni];
        setupUnison (c.aUni, c.aDetune, c.aWidth, uniRA, panLA, panRA);
        setupUnison (c.bUni, c.bDetune, c.bWidth, uniRB, panLB, panRB);
        const float maxDet = 1.0f + 0.04f;
        const int lvA = bank.levelFor (maxFreq * c.aRatio * maxDet, sr);
        const int lvB = bank.levelFor (maxFreq * c.bRatio * maxDet, sr);
        const int lvS = bank.levelFor (maxFreq * c.subRatio, sr);
        const float invSr = 1.0f / sr;
        const float noiseA = 0.02f + 0.98f * c.noiseColour * c.noiseColour;
        const int mode = c.filterType == 0 || c.filterType == 1 ? 0 : (c.filterType == 2 || c.filterType == 3 ? 1 : 2);
        const bool fourPole = c.filterType == 1 || c.filterType == 3;
        const float velGain = 1.0f - c.velSens + c.velSens * vel * vel;
        const float keyOct = c.keyTrack * ((float) note - 60.0f) / 12.0f;

        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, r = 0.0f;
            if (c.aOn && c.levelA[i] > 0.0f)
            {
                float pos = c.waveA[i];
                if (c.lfoDest == 2) pos += c.lfo[i] * c.lfoAmt * 3.5f;
                pos = juce::jlimit (0.0f, 7.0f, pos);
                const float inc = freq[i] * c.aRatio * invSr;
                float sl = 0.0f, sr2 = 0.0f;
                for (int u = 0; u < c.aUni; ++u)
                {
                    const float s = bank.read (pos, lvA, phA[u]);
                    phA[u] += inc * uniRA[u];
                    phA[u] -= std::floor (phA[u]);
                    sl += s * panLA[u]; sr2 += s * panRA[u];
                }
                l += sl * c.levelA[i]; r += sr2 * c.levelA[i];
            }
            if (c.bOn && c.levelB[i] > 0.0f)
            {
                float pos = c.waveB[i];
                if (c.lfoDest == 3) pos += c.lfo[i] * c.lfoAmt * 3.5f;
                pos = juce::jlimit (0.0f, 7.0f, pos);
                const float inc = freq[i] * c.bRatio * invSr;
                float sl = 0.0f, sr2 = 0.0f;
                for (int u = 0; u < c.bUni; ++u)
                {
                    const float s = bank.read (pos, lvB, phB[u]);
                    phB[u] += inc * uniRB[u];
                    phB[u] -= std::floor (phB[u]);
                    sl += s * panLB[u]; sr2 += s * panRB[u];
                }
                l += sl * c.levelB[i]; r += sr2 * c.levelB[i];
            }
            float mono = 0.0f;
            if (c.subLevel[i] > 0.0f)
            {
                const float sine = bank.readFrame (0, lvS, phSub);
                const float sq = bank.readFrame (3, lvS, phSub);
                mono += (sine + c.subShape * (sq * 0.8f - sine)) * c.subLevel[i];
                phSub += freq[i] * c.subRatio * invSr;
                phSub -= std::floor (phSub);
            }
            if (c.noiseLevel[i] > 0.0f)
            {
                const float w = rand01() * 2.0f - 1.0f;
                noiseLp += noiseA * (w - noiseLp);
                mono += noiseLp * c.noiseLevel[i] * (1.6f - 0.8f * c.noiseColour);
            }
            l += mono; r += mono;

            // filter
            const float me = modEnv.next();
            float oct = c.fltEnv * me * 7.0f + keyOct;
            if (c.lfoDest == 1) oct += c.lfo[i] * c.lfoAmt * 4.0f;
            const float fc = juce::jlimit (16.0f, 0.46f * sr, c.cutoff[i] * std::exp2 (oct));
            const float g = std::tan (juce::MathConstants<float>::pi * fc * invSr);
            const float k = 2.0f - 1.94f * c.res[i];
            const float dr = c.drive[i];
            if (dr > 0.001f)
            {
                const float pre = 1.0f + dr * dr * 30.0f;
                const float comp = 1.0f / std::pow (pre, 0.55f);
                l = fastTanh (l * pre) * comp; r = fastTanh (r * pre) * comp;
            }
            l = filt[0].process (l, g, k, mode);
            r = filt[1].process (r, g, k, mode);
            if (fourPole)
            {
                const float k2 = juce::jmin (2.0f, k + 0.6f);
                l = filt[2].process (l, g, k2, mode);
                r = filt[3].process (r, g, k2, mode);
            }

            // amp
            float amp = ampEnv.next() * velGain;
            if (c.lfoDest == 4) amp *= 1.0f - c.lfoAmt * (0.5f - 0.5f * c.lfo[i]);
            float pl = 1.0f, pr = 1.0f;
            if (c.lfoDest == 5)
            {
                const float pan = c.lfo[i] * c.lfoAmt;
                pl = juce::jmin (1.0f, 1.0f - pan); pr = juce::jmin (1.0f, 1.0f + pan);
            }
            outL[i] += l * amp * pl;
            outR[i] += r * amp * pr;
        }
        if (! ampEnv.active()) note = -1;
    }

private:
    float sr = 48000.0f;
    float pitch = 60.0f, targetPitch = 60.0f, vel = 1.0f, penv = 0.0f;
    float phA[kMaxUni] {}, phB[kMaxUni] {}, phSub = 0.0f, noiseLp = 0.0f;
    SVF filt[4];
    Envelope ampEnv, modEnv;
    uint32_t rng = 22222u;

    inline float rand01() noexcept
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) (rng & 0xffffff) / 16777216.0f;
    }

    void resetState() noexcept
    {
        ampEnv.reset(); modEnv.reset();
        for (auto& f : filt) f.reset();
    }

    static void setupUnison (int uni, float detune, float width, float* ratio, float* pl, float* pr) noexcept
    {
        const float spreadCents = detune * detune * 70.0f + detune * 10.0f;  // up to 80 cents each side
        const float norm = 1.0f / std::sqrt ((float) uni);
        for (int u = 0; u < uni; ++u)
        {
            const float pos = uni == 1 ? 0.0f : ((float) u / (float) (uni - 1)) * 2.0f - 1.0f;
            // keep a centred voice in odd counts, put detuned pairs symmetric
            ratio[u] = std::exp2 (pos * spreadCents / 1200.0f);
            const float pan = pos * width;
            const float ang = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            pl[u] = std::cos (ang) * 1.41421356f * norm;
            pr[u] = std::sin (ang) * 1.41421356f * norm;
        }
    }
};
} // namespace xs
