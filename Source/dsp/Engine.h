#pragma once
#include "Voice.h"
#include "../Params.h"
#include <atomic>

namespace xs
{
// Lock-free mono ring buffer the editor reads for the oscilloscope.
struct ScopeBuffer
{
    static constexpr int kSize = 4096;
    float data[kSize] {};
    std::atomic<int> writePos { 0 };
    void push (float v) noexcept
    {
        const int w = writePos.load (std::memory_order_relaxed);
        data[w] = v;
        writePos.store ((w + 1) & (kSize - 1), std::memory_order_release);
    }
};

class Engine
{
public:
    static constexpr int kVoices = 16;

    void bind (juce::AudioProcessorValueTreeState& apvts)
    {
        for (auto& d : paramDefs())
            raw[d.id] = apvts.getRawParameterValue (d.id);
        auto g = [this] (const char* id) { return raw.at (id); };
        pAOn = g ("a_on"); pAWave = g ("a_wave"); pAOct = g ("a_oct"); pASemi = g ("a_semi"); pAFine = g ("a_fine");
        pALevel = g ("a_level"); pAUni = g ("a_uni"); pADet = g ("a_detune"); pAWidth = g ("a_width");
        pBOn = g ("b_on"); pBWave = g ("b_wave"); pBOct = g ("b_oct"); pBSemi = g ("b_semi"); pBFine = g ("b_fine");
        pBLevel = g ("b_level"); pBUni = g ("b_uni"); pBDet = g ("b_detune"); pBWidth = g ("b_width");
        pSubLevel = g ("sub_level"); pSubOct = g ("sub_oct"); pSubShape = g ("sub_shape");
        pNoiseLevel = g ("noise_level"); pNoiseColour = g ("noise_color");
        pFType = g ("flt_type"); pCut = g ("flt_cutoff"); pRes = g ("flt_res"); pFDrive = g ("flt_drive");
        pFEnv = g ("flt_env"); pFKey = g ("flt_key");
        pE1[0] = g ("e1_a"); pE1[1] = g ("e1_d"); pE1[2] = g ("e1_s"); pE1[3] = g ("e1_r");
        pE2[0] = g ("e2_a"); pE2[1] = g ("e2_d"); pE2[2] = g ("e2_s"); pE2[3] = g ("e2_r");
        pPAmt = g ("penv_amt"); pPDecay = g ("penv_decay");
        pLShape = g ("lfo_shape"); pLRate = g ("lfo_rate"); pLSync = g ("lfo_sync"); pLDiv = g ("lfo_div");
        pLDest = g ("lfo_dest"); pLAmt = g ("lfo_amt");
        pMode = g ("voice_mode"); pGlide = g ("glide"); pBend = g ("bend"); pVel = g ("vel_sens");
        pDOn = g ("dist_on"); pDType = g ("dist_type"); pDDrive = g ("dist_drive"); pDTone = g ("dist_tone"); pDMix = g ("dist_mix");
        pCOn = g ("cho_on"); pCRate = g ("cho_rate"); pCDepth = g ("cho_depth"); pCMix = g ("cho_mix");
        pYOn = g ("dly_on"); pYDiv = g ("dly_div"); pYFb = g ("dly_fb"); pYTone = g ("dly_tone"); pYMix = g ("dly_mix"); pYPP = g ("dly_pp");
        pROn = g ("rev_on"); pRSize = g ("rev_size"); pRDamp = g ("rev_damp"); pRWidth = g ("rev_width"); pRMix = g ("rev_mix");
        pVol = g ("master_vol"); pLim = g ("limiter");
    }

    void prepare (double sampleRate, int maxBlock)
    {
        sr = (float) sampleRate;
        for (auto& v : voices) v.prepare (sr);
        ctrl.sr = sr;
        auto initS = [sampleRate] (juce::SmoothedValue<float>& s, float v, double secs = 0.02)
        { s.reset (sampleRate, secs); s.setCurrentAndTargetValue (v); };
        initS (sWaveA, *pAWave); initS (sWaveB, *pBWave); initS (sLevelA, *pALevel * (*pAOn > 0.5f));
        initS (sLevelB, *pBLevel * (*pBOn > 0.5f)); initS (sSub, *pSubLevel); initS (sNoise, *pNoiseLevel);
        initS (sCut, std::log2 ((float) *pCut)); initS (sRes, *pRes); initS (sDrive, *pFDrive);
        initS (sVol, juce::Decibels::decibelsToGain ((float) *pVol), 0.05);
        initS (sDistMix, 0.0f); initS (sChoMix, 0.0f); initS (sDlyMix, 0.0f); initS (sRevMix, 0.0f);
        initS (sDistDrive, *pDDrive); initS (sDlyTime, 0.25f, 0.15);

        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversampler->initProcessing ((size_t) maxBlock);
        chorus.prepare (spec);
        chorus.setCentreDelay (8.0f);
        chorus.setFeedback (0.0f);
        chorus.setMix (1.0f);
        reverb.setSampleRate (sampleRate);
        reverb.reset();
        dlyLen = (int) (sampleRate * 4.5) + 4;
        dlyBuf.setSize (2, dlyLen);
        dlyBuf.clear();
        dlyW = 0;
        dlyLpL = dlyLpR = 0.0f;
        dryBuf.setSize (2, maxBlock);
        wetBuf.setSize (2, maxBlock);
        limGain = 1.0f;
        limLook = juce::jmax (8, (int) (sampleRate * 0.0015));
        limBuf.setSize (2, limLook + 1);
        limBuf.clear();
        limW = 0;
        toneL = toneR = dcL = dcR = dcxL = dcxR = 0.0f;
        lfoPhase = 0.0;
        heldNotes.clear();
        allNotesOff (true);
    }

    int getLatency() const noexcept { return limLook; }

    void setBpm (double b, bool playing, double ppq) noexcept { bpm = b > 20.0 ? b : 150.0; hostPlaying = playing; hostPpq = ppq; }

    void allNotesOff (bool hard)
    {
        for (auto& v : voices) hard ? v.kill() : v.release();
        heldNotes.clear();
    }

    void noteOn (int noteNum, float velocity)
    {
        const int mode = (int) *pMode;
        const bool anyActive = std::any_of (std::begin (voices), std::end (voices), [] (auto& v) { return v.isActive(); });
        if (! anyActive) lfoPhase = 0.0;  // free LFO restarts with a phrase
        refreshCtrl();
        if (mode == 0)
        {
            Voice* v = nullptr;
            for (auto& x : voices) if (x.note == noteNum && x.isActive()) { v = &x; break; }       // same note retrigger
            if (! v) for (auto& x : voices) if (! x.isActive()) { v = &x; break; }
            if (! v) { uint64_t best = UINT64_MAX; for (auto& x : voices) if (x.isReleasing() && x.age < best) { best = x.age; v = &x; } }
            if (! v) { uint64_t best = UINT64_MAX; for (auto& x : voices) if (x.age < best) { best = x.age; v = &x; } }
            v->age = ++ageCounter;
            v->start (noteNum, velocity, -1.0f, false, true, ctrl, (uint32_t) (ageCounter * 2654435761u));
            return;
        }
        // mono / legato: one voice, note stack, glide
        heldNotes.removeFirstMatchingValue (noteNum);
        heldNotes.add (noteNum);
        auto& v = voices[0];
        const bool overlapping = heldNotes.size() > 1 && v.isActive() && ! v.isReleasing();
        const float from = lastPitch >= 0.0f ? (v.isActive() ? v.currentPitch() : lastPitch) : -1.0f;
        if (mode == 2 && overlapping)
            v.legatoTo (noteNum);                                           // 808 slide: no retrigger
        else
            v.start (noteNum, velocity, from, mode == 1 || (mode == 2 && overlapping), true, ctrl, (uint32_t) (++ageCounter * 2654435761u));
        lastPitch = (float) noteNum;
    }

    void noteOff (int noteNum)
    {
        const int mode = (int) *pMode;
        if (mode == 0)
        {
            for (auto& v : voices) if (v.note == noteNum && ! v.isReleasing()) v.release();
            return;
        }
        heldNotes.removeFirstMatchingValue (noteNum);
        auto& v = voices[0];
        if (heldNotes.isEmpty()) { if (v.note == noteNum) v.release(); }
        else if (v.note == noteNum) { v.legatoTo (heldNotes.getLast()); lastPitch = (float) heldNotes.getLast(); }
    }

    void pitchBend (int value14) noexcept { bendNorm = juce::jlimit (-1.0f, 1.0f, ((float) value14 - 8192.0f) / 8192.0f); }

    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) noteOff (m.getNoteNumber());
        else if (m.isPitchWheel()) pitchBend (m.getPitchWheelValue());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff (m.isAllSoundOff());
    }

    void render (juce::AudioBuffer<float>& out, int start, int num)
    {
        auto* L = out.getWritePointer (0, start);
        auto* R = out.getNumChannels() > 1 ? out.getWritePointer (1, start) : nullptr;
        juce::FloatVectorOperations::clear (L, num);
        if (R) juce::FloatVectorOperations::clear (R, num);
        float tmpR[VoiceCtrl::kMax];
        for (int pos = 0; pos < num; pos += VoiceCtrl::kMax)
        {
            const int n = juce::jmin (VoiceCtrl::kMax, num - pos);
            refreshCtrl();
            fillSmoothed (n);
            for (auto& v : voices)
                if (v.isActive())
                {
                    if (! R) { juce::FloatVectorOperations::clear (tmpR, n); v.render (ctrl, L + pos, tmpR); }
                    else v.render (ctrl, L + pos, R + pos);
                }
        }
        // voice headroom
        out.applyGain (start, num, 0.32f);
        if (R) processFx (L, R, num);
    }

    ScopeBuffer scope;
    float outPeak = 0.0f;
    int activeVoices() const { int c = 0; for (auto& v : voices) c += v.isActive(); return c; }

private:
    float sr = 48000.0f;
    std::map<juce::String, std::atomic<float>*> raw;
    std::atomic<float> *pAOn, *pAWave, *pAOct, *pASemi, *pAFine, *pALevel, *pAUni, *pADet, *pAWidth;
    std::atomic<float> *pBOn, *pBWave, *pBOct, *pBSemi, *pBFine, *pBLevel, *pBUni, *pBDet, *pBWidth;
    std::atomic<float> *pSubLevel, *pSubOct, *pSubShape, *pNoiseLevel, *pNoiseColour;
    std::atomic<float> *pFType, *pCut, *pRes, *pFDrive, *pFEnv, *pFKey, *pE1[4], *pE2[4], *pPAmt, *pPDecay;
    std::atomic<float> *pLShape, *pLRate, *pLSync, *pLDiv, *pLDest, *pLAmt, *pMode, *pGlide, *pBend, *pVel;
    std::atomic<float> *pDOn, *pDType, *pDDrive, *pDTone, *pDMix, *pCOn, *pCRate, *pCDepth, *pCMix;
    std::atomic<float> *pYOn, *pYDiv, *pYFb, *pYTone, *pYMix, *pYPP, *pROn, *pRSize, *pRDamp, *pRWidth, *pRMix, *pVol, *pLim;

    Voice voices[kVoices];
    VoiceCtrl ctrl;
    juce::Array<int> heldNotes;
    float lastPitch = -1.0f, bendNorm = 0.0f;
    uint64_t ageCounter = 0;
    double bpm = 150.0, hostPpq = 0.0, lfoPhase = 0.0;
    bool hostPlaying = false;
    float shValue = 0.0f;
    uint32_t shRng = 98765u;

    juce::SmoothedValue<float> sWaveA, sWaveB, sLevelA, sLevelB, sSub, sNoise, sCut, sRes, sDrive, sVol;
    juce::SmoothedValue<float> sDistMix, sChoMix, sDlyMix, sRevMix, sDistDrive, sDlyTime;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;
    juce::AudioBuffer<float> dlyBuf, dryBuf, wetBuf, limBuf;
    int dlyLen = 0, dlyW = 0, limLook = 64, limW = 0;
    float revLp[2][2] {}, dlyF[4] {}, dlyH[2] {}; float dlyLpL = 0, dlyLpR = 0, limGain = 1.0f, toneL = 0, toneR = 0, dcL = 0, dcR = 0, dcxL = 0, dcxR = 0;

    void refreshCtrl() noexcept
    {
        auto& c = ctrl;
        c.aOn = *pAOn > 0.5f; c.bOn = *pBOn > 0.5f;
        c.aUni = juce::jlimit (1, 7, (int) *pAUni); c.bUni = juce::jlimit (1, 7, (int) *pBUni);
        c.aRatio = std::exp2 ((float) *pAOct + (float) *pASemi / 12.0f + (float) *pAFine / 1200.0f);
        c.bRatio = std::exp2 ((float) *pBOct + (float) *pBSemi / 12.0f + (float) *pBFine / 1200.0f);
        c.aDetune = *pADet; c.bDetune = *pBDet; c.aWidth = *pAWidth; c.bWidth = *pBWidth;
        c.subRatio = (int) *pSubOct == 0 ? 0.5f : 0.25f;
        c.subShape = *pSubShape; c.noiseColour = *pNoiseColour;
        c.filterType = (int) *pFType; c.fltEnv = *pFEnv; c.keyTrack = *pFKey;
        c.penvAmt = *pPAmt; c.penvCoef = msToCoef (*pPDecay, sr, 1.0e-3f);
        c.lfoDest = (int) *pLDest; c.lfoAmt = *pLAmt;
        const int mode = (int) *pMode;
        c.glideCoef = (mode > 0 && *pGlide > 0.5f) ? msToCoef (*pGlide, sr, 0.01f) : 0.0f;
        c.bendSemis = bendNorm * (float) *pBend;
        c.velSens = *pVel;
        for (int i = 0; i < 4; ++i) { c.aEnv[i] = *pE1[i]; c.mEnv[i] = *pE2[i]; }
    }

    float lfoValue (double ph) noexcept
    {
        const float p = (float) (ph - std::floor (ph));
        switch ((int) *pLShape)
        {
            case 0: return std::sin (p * juce::MathConstants<float>::twoPi);
            case 1: return 1.0f - 4.0f * std::abs (p - 0.5f);
            case 2: return 1.0f - 2.0f * p;
            case 3: return p < 0.5f ? 1.0f : -1.0f;
            default: return shValue;
        }
    }

    void fillSmoothed (int n) noexcept
    {
        auto& c = ctrl;
        c.n = n;
        sWaveA.setTargetValue (*pAWave); sWaveB.setTargetValue (*pBWave);
        sLevelA.setTargetValue (c.aOn ? (float) *pALevel : 0.0f);
        sLevelB.setTargetValue (c.bOn ? (float) *pBLevel : 0.0f);
        sSub.setTargetValue (*pSubLevel); sNoise.setTargetValue (*pNoiseLevel);
        sCut.setTargetValue (std::log2 ((float) *pCut)); sRes.setTargetValue (*pRes); sDrive.setTargetValue (*pFDrive);
        const bool sync = *pLSync > 0.5f;
        const double rateHz = sync ? (bpm / 60.0) / lfoDivBeats[juce::jlimit (0, 11, (int) *pLDiv)] : (double) *pLRate;
        const double inc = rateHz / sr;
        for (int i = 0; i < n; ++i)
        {
            c.waveA[i] = sWaveA.getNextValue(); c.waveB[i] = sWaveB.getNextValue();
            c.levelA[i] = sLevelA.getNextValue(); c.levelB[i] = sLevelB.getNextValue();
            c.subLevel[i] = sSub.getNextValue(); c.noiseLevel[i] = sNoise.getNextValue();
            c.cutoff[i] = std::exp2 (sCut.getNextValue()); c.res[i] = sRes.getNextValue(); c.drive[i] = sDrive.getNextValue();
            if (sync && hostPlaying)
            {
                const double ppq = hostPpq + (double) i / sr * bpm / 60.0;
                lfoPhase = ppq / lfoDivBeats[juce::jlimit (0, 11, (int) *pLDiv)];
            }
            else
            {
                const double before = lfoPhase;
                lfoPhase += inc;
                if (std::floor (lfoPhase) != std::floor (before))
                {
                    shRng ^= shRng << 13; shRng ^= shRng >> 17; shRng ^= shRng << 5;
                    shValue = (float) (shRng & 0xffff) / 32768.0f - 1.0f;
                }
            }
            c.lfo[i] = lfoValue (lfoPhase);
        }
        if (sync && hostPlaying) hostPpq += (double) n / sr * bpm / 60.0;
    }

    static inline float distort (float x, int type) noexcept
    {
        switch (type)
        {
            case 0: return fastTanh (x);
            case 1: return juce::jlimit (-1.0f, 1.0f, x * 1.8f) * 0.9f;                 // hard clip
            case 2: return std::sin (juce::jlimit (-12.0f, 12.0f, x) * 1.5707963f);      // wavefold
            default:                                                                       // mosh: asymmetric + grit
            {
                const float a = x > 0.0f ? fastTanh (x * 1.6f) : fastTanh (x * 0.7f) * 1.25f;
                return std::round (a * 24.0f) / 24.0f * 0.5f + a * 0.5f;
            }
        }
    }

    void processFx (float* L, float* R, int num)
    {
        dryBuf.copyFrom (0, 0, L, num);
        dryBuf.copyFrom (1, 0, R, num);
        float* chans[2] { L, R };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) num);

        // ---- distortion (2x oversampled)
        sDistMix.setTargetValue (*pDOn > 0.5f ? (float) *pDMix : 0.0f);
        sDistDrive.setTargetValue (*pDDrive);
        if (sDistMix.isSmoothing() || sDistMix.getTargetValue() > 0.0f)
        {
            const int type = (int) *pDType;
            auto up = oversampler->processSamplesUp (block);
            const int un = (int) up.getNumSamples();
            auto* uL = up.getChannelPointer (0); auto* uR = up.getChannelPointer (1);
            float driveStart = sDistDrive.getCurrentValue();
            float driveEnd = sDistDrive.skip (num);
            for (int i = 0; i < un; ++i)
            {
                const float d = driveStart + (driveEnd - driveStart) * (float) i / (float) un;
                const float g = juce::Decibels::decibelsToGain (d * 36.0f);
                const float comp = 1.0f / std::pow (g, type == 2 ? 0.25f : 0.62f);
                uL[i] = distort (uL[i] * g, type) * comp;
                uR[i] = distort (uR[i] * g, type) * comp;
            }
            oversampler->processSamplesDown (block);
            const float toneHz = 800.0f * std::pow (25.0f, (float) *pDTone);
            const float a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * toneHz / sr);
            for (int i = 0; i < num; ++i)
            {
                toneL += a * (L[i] - toneL); toneR += a * (R[i] - toneR);
                // DC blocker
                const float yl = toneL - dcxL + 0.9995f * dcL; dcxL = toneL; dcL = yl;
                const float yr = toneR - dcxR + 0.9995f * dcR; dcxR = toneR; dcR = yr;
                const float m = sDistMix.getNextValue();
                L[i] = dryBuf.getSample (0, i) + m * (yl - dryBuf.getSample (0, i));
                R[i] = dryBuf.getSample (1, i) + m * (yr - dryBuf.getSample (1, i));
            }
        }
        // ---- chorus
        sChoMix.setTargetValue (*pCOn > 0.5f ? (float) *pCMix : 0.0f);
        if (sChoMix.isSmoothing() || sChoMix.getTargetValue() > 0.0f)
        {
            wetBuf.copyFrom (0, 0, L, num); wetBuf.copyFrom (1, 0, R, num);
            chorus.setRate (*pCRate);
            chorus.setDepth (0.05f + 0.95f * (float) *pCDepth);
            juce::dsp::AudioBlock<float> wb (wetBuf.getArrayOfWritePointers(), 2, (size_t) num);
            chorus.process (juce::dsp::ProcessContextReplacing<float> (wb));
            for (int i = 0; i < num; ++i)
            {
                const float m = sChoMix.getNextValue();
                L[i] += m * (wetBuf.getSample (0, i) - L[i]);
                R[i] += m * (wetBuf.getSample (1, i) - R[i]);
            }
        }
        // ---- tempo-synced delay
        sDlyMix.setTargetValue (*pYOn > 0.5f ? (float) *pYMix : 0.0f);
        sDlyTime.setTargetValue ((float) (60.0 / bpm * delayDivBeats[juce::jlimit (0, 10, (int) *pYDiv)]));
        if (sDlyMix.isSmoothing() || sDlyMix.getTargetValue() > 0.0f)
        {
            const float fb = *pYFb;
            const bool pp = *pYPP > 0.5f;
            // Tone: left half darkens the echoes (12 dB/oct LP 16 kHz -> 450 Hz),
            // right half thins them out (HP 20 Hz -> 1.6 kHz) for telephone-style repeats.
            const float tone = *pYTone;
            const float lpHz = tone < 0.5f ? 450.0f * std::pow (16000.0f / 450.0f, tone * 2.0f) : 16000.0f;
            const float hpHz = tone > 0.5f ? 20.0f * std::pow (1600.0f / 20.0f, (tone - 0.5f) * 2.0f) : 20.0f;
            const float a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * lpHz / sr);
            const float ah = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hpHz / sr);
            auto* dL = dlyBuf.getWritePointer (0); auto* dR = dlyBuf.getWritePointer (1);
            for (int i = 0; i < num; ++i)
            {
                const float dSamp = juce::jlimit (1.0f, (float) dlyLen - 2.0f, sDlyTime.getNextValue() * sr);
                double rp = (double) dlyW - (double) dSamp;
                if (rp < 0.0) rp += (double) dlyLen;
                int i0 = (int) rp;
                const float t = (float) (rp - (double) i0);
                if (i0 >= dlyLen) i0 -= dlyLen;           // guard float rounding at the wrap point
                const int i1 = (i0 + 1) % dlyLen;
                const float yl = dL[i0] + t * (dL[i1] - dL[i0]);
                const float yr = dR[i0] + t * (dR[i1] - dR[i0]);
                dlyF[0] += a * (yl - dlyF[0]);       dlyF[1] += a * (dlyF[0] - dlyF[1]);
                dlyF[2] += a * (yr - dlyF[2]);       dlyF[3] += a * (dlyF[2] - dlyF[3]);
                dlyH[0] += ah * (dlyF[1] - dlyH[0]); dlyH[1] += ah * (dlyF[3] - dlyH[1]);
                dlyLpL = dlyF[1] - dlyH[0];
                dlyLpR = dlyF[3] - dlyH[1];
                if (pp)
                {
                    const float in = 0.5f * (L[i] + R[i]);
                    dL[dlyW] = fastTanh (in + dlyLpR * fb);
                    dR[dlyW] = fastTanh (dlyLpL * fb);
                }
                else
                {
                    dL[dlyW] = fastTanh (L[i] + dlyLpL * fb);
                    dR[dlyW] = fastTanh (R[i] + dlyLpR * fb);
                }
                dlyW = (dlyW + 1) % dlyLen;
                const float m = sDlyMix.getNextValue();
                L[i] += m * 1.2f * dlyLpL - m * 0.3f * L[i];
                R[i] += m * 1.2f * dlyLpR - m * 0.3f * R[i];
            }
        }
        else if (dlyLpL != 0.0f || dlyLpR != 0.0f) { dlyBuf.clear(); dlyLpL = dlyLpR = 0.0f; for (auto& x : dlyF) x = 0; dlyH[0] = dlyH[1] = 0; }
        // ---- reverb
        sRevMix.setTargetValue (*pROn > 0.5f ? (float) *pRMix : 0.0f);
        if (sRevMix.isSmoothing() || sRevMix.getTargetValue() > 0.0f)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = 0.3f + 0.69f * (float) *pRSize;
            rp.damping = *pRDamp;
            rp.width = *pRWidth;
            rp.wetLevel = 1.0f / 3.0f;   // unity wet (juce scales wet by 3)
            rp.dryLevel = 0.0f;
            reverb.setParameters (rp);
            wetBuf.copyFrom (0, 0, L, num); wetBuf.copyFrom (1, 0, R, num);
            reverb.processStereo (wetBuf.getWritePointer (0), wetBuf.getWritePointer (1), num);
            // Damp also darkens the wet signal (12 dB/oct, 20 kHz -> 600 Hz) so the knob is clearly audible
            const float dampHz = 20000.0f * std::pow (600.0f / 20000.0f, std::pow ((float) *pRDamp, 0.6f));
            const float da = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * dampHz / sr);
            for (int i = 0; i < num; ++i)
            {
                for (int ch = 0; ch < 2; ++ch)
                {
                    float x = wetBuf.getSample (ch, i);
                    revLp[ch][0] += da * (x - revLp[ch][0]);
                    revLp[ch][1] += da * (revLp[ch][0] - revLp[ch][1]);
                    wetBuf.setSample (ch, i, revLp[ch][1]);
                }
                const float m = sRevMix.getNextValue();
                const float dry = 1.0f - 0.5f * m;
                L[i] = L[i] * dry + wetBuf.getSample (0, i) * m * 0.9f;
                R[i] = R[i] * dry + wetBuf.getSample (1, i) * m * 0.9f;
            }
        }
        // ---- master + lookahead limiter
        sVol.setTargetValue (juce::Decibels::decibelsToGain ((float) *pVol));
        const bool lim = *pLim > 0.5f;
        constexpr float ceiling = 0.944f; // -0.5 dBFS
        const float rel = std::exp (-1.0f / (0.12f * sr));
        const float att = std::exp (-1.0f / (0.25f * (float) limLook));
        auto* bL = limBuf.getWritePointer (0); auto* bR = limBuf.getWritePointer (1);
        const int lb = limBuf.getNumSamples();
        float peak = 0.0f;
        for (int i = 0; i < num; ++i)
        {
            const float g = sVol.getNextValue();
            float xl = L[i] * g, xr = R[i] * g;
            if (! std::isfinite (xl)) xl = 0.0f;
            if (! std::isfinite (xr)) xr = 0.0f;
            const float lvl = juce::jmax (std::abs (xl), std::abs (xr));
            const float target = lvl > ceiling ? ceiling / lvl : 1.0f;
            limGain = target < limGain ? target + att * (limGain - target) : target + rel * (limGain - target);
            bL[limW] = xl; bR[limW] = xr;
            int rpos = limW - limLook; if (rpos < 0) rpos += lb;
            float ol = bL[rpos], orr = bR[rpos];
            limW = (limW + 1) % lb;
            if (lim)
            {
                ol = softClip (ol * limGain, ceiling);
                orr = softClip (orr * limGain, ceiling);
            }
            L[i] = ol; R[i] = orr;
            peak = juce::jmax (peak, std::abs (ol), std::abs (orr));
            scope.push (0.5f * (ol + orr));
        }
        outPeak = peak;
    }

    static inline float softClip (float x, float c) noexcept
    {
        const float t = 0.75f * c, a = std::abs (x);
        if (a <= t) return x;
        const float y = t + (c - t) * fastTanh ((a - t) / (c - t));
        return x < 0 ? -y : y;
    }
};
} // namespace xs
