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
    // v1.1 drum / vox engine
    float noiseDecay = 0.0f;          // ms; 0 = noise rides the amp env
    float ring = 0.0f;                // osc A x osc B ring modulation
    int burst = 0;                    // clap pre-hits
    float burstGap = 10.0f;           // ms between clap hits
    bool voxOn = false;
    float vowel = 0.0f, voxEnv = 0.0f, voxMix = 1.0f;
    // v1.2 808 engine / light synths
    float punch = 0.0f;               // 0-1: pitch drop (up to +24 st) + attack boost (up to +5.4 dB) on each new hit
    float click = 0.0f;               // 0-1: click / kick transient layer
    float lfoFade = 0.0f;             // ms: per-note LFO fade-in (delayed vibrato)
};

// Vowel formants (F1-F3 Hz, gain, bandwidth Hz) for A E I O U.
struct FormantTable
{
    static constexpr float f[5][3]  { { 650, 1080, 2650 }, { 400, 1700, 2600 }, { 290, 1870, 2800 }, { 400, 800, 2600 }, { 350, 600, 2700 } };
    static constexpr float g[5][3]  { { 1.0f, 0.50f, 0.45f }, { 1.0f, 0.20f, 0.25f }, { 1.0f, 0.18f, 0.13f }, { 1.0f, 0.32f, 0.25f }, { 1.0f, 0.10f, 0.14f } };
    static constexpr float bw[5][3] { { 80, 90, 120 }, { 70, 80, 100 }, { 40, 90, 100 }, { 40, 80, 100 }, { 40, 60, 100 } };
};

class Voice
{
public:
    static constexpr int kMaxUni = 7;

    void prepare (float sampleRate) noexcept { sr = sampleRate; resetState(); }

    bool isActive() const noexcept { return ampEnv.active() || noiseEnv.active() || burstLeft > 0; }
    bool isReleasing() const noexcept { return ampEnv.state == Envelope::Release || (! ampEnv.active() && isActive()); }
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
            for (auto& f : nFilt) f.reset();
            for (auto& ch : fmt) for (auto& f : ch) f.reset();
            noiseLp = 0.0f;
        }
        autoRel = -1;
        if (retrigger || fresh)
        {
            updateEnvs (c);
            burstLen = juce::jmax (1, (int) (c.burstGap * 0.001f * sr));
            burstLeft = c.burst * burstLen;
            if (burstLeft > 0) { ampEnv.reset(); noiseEnv.reset(); }   // envelopes start after the clap pre-hits
            else { ampEnv.noteOn(); noiseEnv.noteOn(); }
            modEnv.noteOn();
            penv = 1.0f;
            punchP = punchA = 1.0f;
            clickPos = 0; clickPh = 0.0f;
            fadePos = 0;
        }
    }

    // one-shot mode: release by itself after attack + decay (+ clap pre-hits), whatever the MIDI note length
    void setAutoRelease (const VoiceCtrl& c) noexcept
    {
        autoRel = (int) ((c.aEnv[0] + c.aEnv[1] + (float) c.burst * c.burstGap) * 0.001f * sr);
    }
    void legatoTo (int midiNote) noexcept { note = midiNote; targetPitch = (float) midiNote; }
    void release() noexcept { if (burstLeft > 0) { burstLeft = 0; ampEnv.noteOn(); noiseEnv.noteOn(); } ampEnv.noteOff(); modEnv.noteOff(); noiseEnv.noteOff(); }
    void kill() noexcept { autoRel = -1; clickPos = -1; ampEnv.reset(); modEnv.reset(); noiseEnv.reset(); burstLeft = 0; note = -1; }
    float currentPitch() const noexcept { return pitch; }

    void updateEnvs (const VoiceCtrl& c) noexcept
    {
        ampEnv.set (c.aEnv[0], c.aEnv[1], c.aEnv[2], c.aEnv[3], sr);
        modEnv.set (c.mEnv[0], c.mEnv[1], c.mEnv[2], c.mEnv[3], sr);
        noiseEnv.set (0.0f, juce::jmax (1.0f, c.noiseDecay), 0.0f, juce::jmin (c.aEnv[3], c.noiseDecay + 4.0f), sr);
    }

    void render (const VoiceCtrl& c, float* outL, float* outR) noexcept
    {
        const int n = c.n;
        const auto& bank = WavetableBank::get();
        updateEnvs (c);
        if (autoRel >= 0) { autoRel -= n; if (autoRel < 0) { autoRel = -1; ampEnv.noteOff(); modEnv.noteOff(); } }   // noise layer keeps its own decay

        // per-note LFO fade-in (delayed vibrato for flutes / leads)
        float lfoAmt = c.lfoAmt;
        if (c.lfoFade > 1.0f)
        {
            lfoAmt *= juce::jmin (1.0f, (float) fadePos / (c.lfoFade * 0.001f * sr));
            fadePos += n;
        }
        // punch: pitch drop tau 12-42 ms, attack boost tau 30 ms
        const float punchCoefP = std::exp (-1.0f / ((0.012f + 0.03f * c.punch) * sr));
        const float punchCoefA = std::exp (-1.0f / (0.03f * sr));
        float boost[VoiceCtrl::kMax];

        // pass 1: pitch per sample (glide, pitch env, punch, bend, vibrato)
        float freq[VoiceCtrl::kMax];
        float maxFreq = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            if (c.glideCoef > 0.0f) pitch = targetPitch + (pitch - targetPitch) * c.glideCoef;
            else pitch = targetPitch;
            penv *= c.penvCoef;
            punchP *= punchCoefP; punchA *= punchCoefA;
            boost[i] = 1.0f + 0.85f * c.punch * punchA;
            float p = pitch + c.penvAmt * penv + c.bendSemis + c.punch * 24.0f * punchP;
            if (c.lfoDest == 0) p += c.lfo[i] * lfoAmt * lfoAmt * 12.0f;
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
        const bool sepNoise = c.noiseDecay > 0.5f;
        // formant (vowel) filter, coefficients per chunk
        const bool vox = c.voxOn && c.voxMix > 0.0f;
        float fg[3] {}, fk[3] {}, fa[3] {};
        if (vox)
        {
            float vw = c.vowel + c.voxEnv * 4.0f * modEnv.level;
            if (c.lfoDest == 6) vw += c.lfo[0] * lfoAmt * 2.0f;
            vw = juce::jlimit (0.0f, 4.0f, vw);
            const int i0 = juce::jmin (3, (int) vw);
            const float t = vw - (float) i0;
            const float formantShift = std::exp2 (juce::jlimit (-1.0f, 1.0f, ((float) note - 60.0f) / 48.0f)); // gentle tracking
            for (int k = 0; k < 3; ++k)
            {
                const float fr = (FormantTable::f[i0][k] + t * (FormantTable::f[i0 + 1][k] - FormantTable::f[i0][k])) * formantShift;
                const float b  = FormantTable::bw[i0][k] + t * (FormantTable::bw[i0 + 1][k] - FormantTable::bw[i0][k]);
                fa[k] = FormantTable::g[i0][k] + t * (FormantTable::g[i0 + 1][k] - FormantTable::g[i0][k]);
                fg[k] = std::tan (juce::MathConstants<float>::pi * juce::jmin (fr, 0.45f * sr) * invSr);
                fk[k] = juce::jlimit (0.02f, 2.0f, (b * 1.5f) / fr);
            }
        }

        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, r = 0.0f;
            float aL = 0.0f, aR = 0.0f;
            if (c.aOn && c.levelA[i] > 0.0f)
            {
                float pos = c.waveA[i];
                if (c.lfoDest == 2) pos += c.lfo[i] * lfoAmt * 3.5f;
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
                aL = sl * c.levelA[i]; aR = sr2 * c.levelA[i];
            }
            if (c.bOn && c.levelB[i] > 0.0f)
            {
                float pos = c.waveB[i];
                if (c.lfoDest == 3) pos += c.lfo[i] * lfoAmt * 3.5f;
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
            if (c.ring > 0.0f)
            {   // ring mod: metallic clangs for hats, crashes, bells and percs
                const float rl = aL * l * 2.0f, rr = aR * r * 2.0f;
                l = (aL + l) * (1.0f - c.ring) + rl * c.ring;
                r = (aR + r) * (1.0f - c.ring) + rr * c.ring;
            }
            else { l += aL; r += aR; }
            float mono = 0.0f, nz = 0.0f;
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
                const float ns = noiseLp * c.noiseLevel[i] * (1.6f - 0.8f * c.noiseColour);
                if (sepNoise) nz = ns; else mono += ns;
            }
            l += mono; r += mono;

            // filter
            const float me = modEnv.next();
            float oct = c.fltEnv * me * 7.0f + keyOct;
            if (c.lfoDest == 1) oct += c.lfo[i] * lfoAmt * 4.0f;
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
            if (sepNoise)
            {   // noise layer with its own decay (snare wires, kick click, hat sizzle) through the same filter
                if (dr > 0.001f) { const float pre = 1.0f + dr * dr * 30.0f; nz = fastTanh (nz * pre) / std::pow (pre, 0.55f); }
                nz = nFilt[0].process (nz, g, k, mode);
                if (fourPole) nz = nFilt[1].process (nz, g, juce::jmin (2.0f, k + 0.6f), mode);
            }
            if (vox)
            {
                float yl = 0.0f, yr = 0.0f;
                for (int q = 0; q < 3; ++q)
                {
                    yl += fmt[0][q].process (l, fg[q], fk[q], 2) * fa[q];
                    yr += fmt[1][q].process (r, fg[q], fk[q], 2) * fa[q];
                }
                l += c.voxMix * (yl * 2.2f - l);
                r += c.voxMix * (yr * 2.2f - r);
            }

            // clap pre-hits: short decaying bursts, then the envelopes start
            float burstGain = 1.0f;
            bool inBurst = false;
            if (burstLeft > 0)
            {
                inBurst = true;
                const int segPos = burstLen - 1 - ((burstLeft - 1) % burstLen);
                burstGain = std::exp (-(float) segPos / (0.28f * (float) burstLen));
                if (--burstLeft == 0) { ampEnv.noteOn(); noiseEnv.noteOn(); }
            }

            // amp
            float amp = (inBurst ? burstGain : ampEnv.next()) * velGain * boost[i];
            const float nAmp = sepNoise ? (inBurst ? burstGain : noiseEnv.next()) * velGain : 0.0f;
            if (c.lfoDest == 4) amp *= 1.0f - lfoAmt * (0.5f - 0.5f * c.lfo[i]);
            float pl = 1.0f, pr = 1.0f;
            if (c.lfoDest == 5)
            {
                const float pan = c.lfo[i] * lfoAmt;
                pl = juce::jmin (1.0f, 1.0f - pan); pr = juce::jmin (1.0f, 1.0f + pan);
            }
            // click / kick layer: a fast 3.6 kHz -> 150 Hz sine sweep plus a noise tick, longer and knockier as Click goes up
            float ck = 0.0f;
            if (clickPos >= 0)
            {
                if (c.click > 0.0f)
                {
                    const float t = (float) clickPos * invSr;
                    const float cf = 150.0f + 3500.0f * std::exp (-t / 0.004f);
                    clickPh += cf * invSr; clickPh -= std::floor (clickPh);
                    const float body = std::sin (clickPh * juce::MathConstants<float>::twoPi) * std::exp (-t / (0.005f + 0.022f * c.click));
                    const float tick = (rand01() * 2.0f - 1.0f) * std::exp (-t / 0.0012f) * 0.6f;
                    ck = (body + tick) * c.click * 1.5f * velGain;
                }
                if (++clickPos > (int) (0.15f * sr)) clickPos = -1;
            }
            outL[i] += l * amp * pl + nz * nAmp + ck;
            outR[i] += r * amp * pr + nz * nAmp + ck;
        }
        if (! sepNoise) noiseEnv.reset();
        if (! isActive()) note = -1;
    }

private:
    float sr = 48000.0f;
    float pitch = 60.0f, targetPitch = 60.0f, vel = 1.0f, penv = 0.0f, punchP = 0.0f, punchA = 0.0f, clickPh = 0.0f;
    int clickPos = -1, fadePos = 0;
    float phA[kMaxUni] {}, phB[kMaxUni] {}, phSub = 0.0f, noiseLp = 0.0f;
    SVF filt[4], nFilt[2], fmt[2][3];
    Envelope ampEnv, modEnv, noiseEnv;
    int burstLeft = 0, burstLen = 441, autoRel = -1;
    uint32_t rng = 22222u;

    inline float rand01() noexcept
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) (rng & 0xffffff) / 16777216.0f;
    }

    void resetState() noexcept
    {
        ampEnv.reset(); modEnv.reset(); noiseEnv.reset(); burstLeft = 0;
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
