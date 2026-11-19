// Offline test harness for the Xoxicoa Synth engine (runs the real processor).
//   XoxicoaSynthAudit audit  <report.txt>     parameter sweep: min / mid / max
//   XoxicoaSynthAudit safety <report.txt>     NaN / clipping / clicks / CPU / presets / generator
//   XoxicoaSynthAudit demos  <outDir>         render demo WAVs
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>

namespace
{
constexpr double kFs = 48000.0;
constexpr int kBlock = 256;

struct PlayHead : juce::AudioPlayHead
{
    double bpm = 160.0; juce::int64 sample = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm); p.setIsPlaying (true); p.setTimeInSamples (sample);
        p.setPpqPosition ((double) sample / kFs * bpm / 60.0);
        p.setTimeSignature (TimeSignature { 4, 4 });
        return p;
    }
};

struct Ev { double t; int note; float vel; bool on; int bend = -1; };
using Phrase = std::vector<Ev>;

void note (Phrase& p, double t0, double t1, int n, float vel = 0.8f) { p.push_back ({ t0, n, vel, true }); p.push_back ({ t1, n, 0.0f, false }); }

juce::AudioBuffer<float> render (XoxicoaSynthProcessor& proc, Phrase ph, double seconds, double bpm = 160.0, double* cpuSecs = nullptr)
{
    PlayHead head; head.bpm = bpm;
    proc.setPlayHead (&head);
    proc.prepareToPlay (kFs, kBlock);
    std::sort (ph.begin(), ph.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && ! a.on && b.on); });
    const int total = (int) (seconds * kFs);
    juce::AudioBuffer<float> out (2, total), blk (2, kBlock);
    size_t ei = 0;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (int pos = 0; pos < total; pos += kBlock)
    {
        const int n = juce::jmin (kBlock, total - pos);
        juce::MidiBuffer midi;
        while (ei < ph.size() && (int) (ph[ei].t * kFs) < pos + n)
        {
            const auto& e = ph[ei++];
            const int at = juce::jlimit (0, n - 1, (int) (e.t * kFs) - pos);
            if (e.bend >= 0) midi.addEvent (juce::MidiMessage::pitchWheel (1, e.bend), at);
            else if (e.on) midi.addEvent (juce::MidiMessage::noteOn (1, e.note, e.vel), at);
            else midi.addEvent (juce::MidiMessage::noteOff (1, e.note), at);
        }
        blk.setSize (2, n, false, false, true);
        head.sample = pos;
        proc.processBlock (blk, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, blk, c, 0, n);
    }
    if (cpuSecs) *cpuSecs = (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0;
    proc.setPlayHead (nullptr);
    // drop the limiter's lookahead latency so measurements line up with the notes
    const int lat = proc.getLatencySamples();
    juce::AudioBuffer<float> aligned (2, total);
    aligned.clear();
    for (int c = 0; c < 2; ++c) aligned.copyFrom (c, 0, out, c, lat, total - lat);
    return aligned;
}

void setP (XoxicoaSynthProcessor& proc, const juce::String& id, float v)
{
    auto* p = proc.apvts.getParameter (id);
    jassert (p != nullptr);
    p->setValueNotifyingHost (p->convertTo0to1 (v));
}

// ---------------- analysis
struct Features
{
    float rmsDb = -200, peakDb = -200, sideDb = 0;
    std::vector<float> spec;   // 1/12-octave band levels (dB) of the mid signal
    std::vector<float> env;    // 20 ms RMS frames (dB)
    std::vector<float> bands;  // 20 ms frames x 3 bands (<300 Hz, 300-3k, >3k), dB
    bool finite = true;
};

Features analyse (const juce::AudioBuffer<float>& b)
{
    Features f;
    const int n = b.getNumSamples();
    const float* L = b.getReadPointer (0); const float* R = b.getReadPointer (1);
    double s2 = 0, m2 = 0, d2 = 0; float pk = 0;
    for (int i = 0; i < n; ++i)
    {
        if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) { f.finite = false; continue; }
        const double m = 0.5 * (L[i] + R[i]), d = 0.5 * (L[i] - R[i]);
        s2 += 0.5 * (L[i] * (double) L[i] + R[i] * (double) R[i]); m2 += m * m; d2 += d * d;
        pk = juce::jmax (pk, std::abs (L[i]), std::abs (R[i]));
    }
    f.rmsDb = (float) (10.0 * std::log10 (s2 / n + 1e-20));
    f.peakDb = juce::Decibels::gainToDecibels (pk, -200.0f);
    f.sideDb = (float) juce::jlimit (-40.0, 6.0, 10.0 * std::log10 ((d2 + 1e-12) / (m2 + 1e-12)));  // mono clamps to -40
    // envelope
    const int fr = (int) (0.02 * kFs);
    for (int s = 0; s + fr <= n; s += fr)
    {
        double e = 0;
        for (int i = s; i < s + fr; ++i) e += 0.5 * (L[i] * (double) L[i] + R[i] * (double) R[i]);
        f.env.push_back ((float) juce::jmax (-90.0, 10.0 * std::log10 (e / fr + 1e-20)));
    }
    // spectro-temporal: 3 bands per 20 ms frame (catches tails, echoes, filter sweeps)
    {
        juce::IIRFilter lp, hp, lp3;
        lp.setCoefficients (juce::IIRCoefficients::makeLowPass (kFs, 300.0));
        lp3.setCoefficients (juce::IIRCoefficients::makeLowPass (kFs, 3000.0));
        hp.setCoefficients (juce::IIRCoefficients::makeHighPass (kFs, 3000.0));
        double e[3] {}; int k = 0;
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            const float lo = lp.processSingleSampleRaw (m), mid3 = lp3.processSingleSampleRaw (m), hi = hp.processSingleSampleRaw (m);
            e[0] += lo * (double) lo; e[1] += (mid3 - lo) * (double) (mid3 - lo); e[2] += hi * (double) hi;
            if (++k == fr)
            {
                for (auto& x : e) { f.bands.push_back ((float) juce::jmax (-90.0, 10.0 * std::log10 (x / fr + 1e-20))); x = 0; }
                k = 0;
            }
        }
    }
    // averaged spectrum
    constexpr int order = 13, N = 1 << order;
    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> win (N, juce::dsp::WindowingFunction<float>::hann);
    std::vector<double> pow (N / 2, 0.0);
    std::vector<float> buf (N * 2);
    int frames = 0;
    for (int s = 0; s + N <= n; s += N / 2, ++frames)
    {
        for (int i = 0; i < N; ++i) buf[(size_t) i] = 0.5f * (L[s + i] + R[s + i]);
        std::fill (buf.begin() + N, buf.end(), 0.0f);
        win.multiplyWithWindowingTable (buf.data(), N);
        fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int k = 0; k < N / 2; ++k) pow[(size_t) k] += buf[(size_t) k] * (double) buf[(size_t) k];
    }
    for (double fc = 30.0; fc < 18000.0; fc *= std::pow (2.0, 1.0 / 12.0))
    {
        const int k0 = (int) (fc * std::pow (2.0, -1.0 / 24.0) / kFs * N), k1 = juce::jmax (k0 + 1, (int) (fc * std::pow (2.0, 1.0 / 24.0) / kFs * N));
        double e = 0;
        for (int k = k0; k < k1 && k < N / 2; ++k) e += pow[(size_t) k];
        f.spec.push_back ((float) juce::jmax (-100.0, 10.0 * std::log10 (e / juce::jmax (1, frames) + 1e-20)));
    }
    return f;
}

struct Dist { float rms, spec, env, side, peak, band; float score() const { return std::max ({ std::abs (rms), spec, env, std::abs (side) * 0.5f, std::abs (peak), band }); } };

Dist distance (const Features& a, const Features& b)
{
    Dist d {};
    d.rms = b.rmsDb - a.rmsDb;
    d.peak = b.peakDb - a.peakDb;
    d.side = b.sideDb - a.sideDb;
    // spectral shape: level-normalised, only bands that matter (within 60 dB of the loudest)
    float ma = -200, mb = -200;
    for (size_t i = 0; i < a.spec.size(); ++i) { ma = juce::jmax (ma, a.spec[i]); mb = juce::jmax (mb, b.spec[i]); }
    double acc = 0; int cnt = 0;
    for (size_t i = 0; i < a.spec.size(); ++i)
    {
        const float x = juce::jmax (a.spec[i] - ma, -60.0f), y = juce::jmax (b.spec[i] - mb, -60.0f);
        if (x > -60.0f || y > -60.0f) { acc += std::abs (x - y); ++cnt; }
    }
    d.spec = cnt ? (float) (acc / cnt) : 0.0f;
    acc = 0; cnt = 0;
    for (size_t i = 0; i < juce::jmin (a.env.size(), b.env.size()); ++i)
        if (a.env[i] > -70.0f || b.env[i] > -70.0f) { acc += std::abs (juce::jmax (a.env[i], -70.0f) - juce::jmax (b.env[i], -70.0f)); ++cnt; }
    d.env = cnt ? (float) (acc / cnt) : 0.0f;
    acc = 0; cnt = 0;
    for (size_t i = 0; i < juce::jmin (a.bands.size(), b.bands.size()); ++i)
        if (a.bands[i] > -70.0f || b.bands[i] > -70.0f) { acc += std::abs (juce::jmax (a.bands[i], -70.0f) - juce::jmax (b.bands[i], -70.0f)); ++cnt; }
    d.band = cnt ? (float) (acc / cnt) : 0.0f;
    return d;
}

// ---------------- audit contexts
xs::Patch auditBase()
{
    auto p = xs::initPatch();
    p["master_vol"] = -6.0f;
    p["limiter"] = 0.0f;  // measure the raw engine, not the limiter
    return p;
}

Phrase polyPhrase()
{
    Phrase p;
    note (p, 0.00, 0.70, 48); note (p, 0.00, 0.70, 51); note (p, 0.00, 0.70, 55);
    note (p, 0.80, 1.30, 60, 0.9f); note (p, 1.35, 1.60, 67, 0.7f);
    return p;
}

Phrase slidePhrase()
{
    Phrase p;
    note (p, 0.00, 0.45, 36, 0.9f); note (p, 0.40, 0.95, 43, 0.9f); note (p, 0.90, 1.50, 39, 0.9f);
    return p;
}

struct Context { xs::Patch patch; Phrase phrase; double secs = 2.4; };

Context contextFor (const juce::String& id)
{
    Context c { auditBase(), polyPhrase() };
    auto& p = c.patch;
    auto starts = [&id] (const char* s) { return id.startsWith (s); };
    if (starts ("a_")) { if (id == "a_detune" || id == "a_width") p["a_uni"] = 5; }
    else if (starts ("b_")) { p["a_on"] = 0; p["b_on"] = 1; if (id == "b_detune" || id == "b_width") p["b_uni"] = 5; }
    else if (starts ("sub_")) { p["a_on"] = 0; p["sub_level"] = 0.8f; }
    else if (starts ("noise_")) { p["a_on"] = 0; p["noise_level"] = 0.7f; }
    else if (starts ("flt_"))
    {
        p["flt_cutoff"] = 1500;
        if (id == "flt_env") { p["flt_cutoff"] = 400; p["e2_d"] = 250; p["e2_s"] = 0.0f; }
        if (id == "flt_key") { p["flt_cutoff"] = 600; }
        if (id == "flt_drive") { p["a_level"] = 0.5f; }
        if (id == "flt_res") { p["flt_cutoff"] = 1000; }
    }
    else if (starts ("e1_")) { if (id == "e1_s") p["e1_d"] = 150; if (id == "e1_d") p["e1_s"] = 0.2f; }
    else if (starts ("e2_")) { p["e1_r"] = 2500; p["flt_cutoff"] = 250; p["flt_env"] = 0.8f; p["flt_type"] = 1; if (id == "e2_s") p["e2_d"] = 150; if (id == "e2_d") p["e2_s"] = 0.1f; }
    else if (starts ("penv_")) { if (id == "penv_decay") p["penv_amt"] = 12; else p["penv_decay"] = 80; }
    else if (starts ("lfo_"))
    {
        p["lfo_amt"] = 0.5f; p["lfo_dest"] = 1; p["flt_cutoff"] = 1200; p["lfo_rate"] = 3;
        if (id == "lfo_div") p["lfo_sync"] = 1;
        if (id == "lfo_shape") p["lfo_rate"] = 2;
        if (id == "lfo_fade") { p["lfo_dest"] = 0; p["lfo_rate"] = 5.5f; p["lfo_amt"] = 0.4f; }
    }
    else if (id == "voice_mode") { p["glide"] = 200; p["e1_d"] = 200; p["e1_s"] = 0.3f; p["penv_amt"] = 12; c.phrase = slidePhrase(); }
    else if (id == "glide") { p["voice_mode"] = 2; c.phrase = slidePhrase(); }
    else if (id == "bend") { c.phrase.push_back ({ 0.3, 0, 0, false, 16383 }); }
    else if (id == "vel_sens") { for (auto& e : c.phrase) if (e.on) e.vel = 0.25f; }
    else if (starts ("dist_")) { p["dist_on"] = 1; p["dist_drive"] = 0.5f; }
    else if (starts ("cho_")) p["cho_on"] = 1;
    else if (starts ("dly_") || starts ("rev_"))
    {
        // staccato hits so the echoes / tail are exposed
        p[starts ("dly_") ? "dly_on" : "rev_on"] = 1;
        p["e1_s"] = 0.0f; p["e1_d"] = 90; p["e1_r"] = 60;
        c.phrase.clear();
        note (c.phrase, 0.0, 0.08, 60); note (c.phrase, 1.2, 1.28, 67);
        c.secs = 3.0;
    }
    else if (id == "punch" || id == "click" || id == "low_keep")
    {   // an 808: sine at the real pitch, low notes
        p["a_wave"] = 0; p["e1_d"] = 1500; p["e1_s"] = 0.6f; p["flt_cutoff"] = 9000;
        if (id == "low_keep") { p["dist_on"] = 1; p["dist_type"] = 2; p["dist_drive"] = 0.75f; p["dist_tone"] = 0.8f; }
        c.phrase.clear();
        note (c.phrase, 0.0, 0.6, 33); note (c.phrase, 0.8, 1.4, 36); note (c.phrase, 1.5, 1.9, 40);
        c.secs = 2.2;
    }
    else if (id == "burst" || id == "burst_gap")
    {   // a clap: noise bursts
        p["a_on"] = 0; p["noise_level"] = 0.9f; p["noise_color"] = 0.8f; p["e1_d"] = 120; p["e1_s"] = 0.0f; p["one_shot"] = 1;
        if (id == "burst_gap") p["burst"] = 3;
        c.phrase.clear(); note (c.phrase, 0.0, 0.3, 60); note (c.phrase, 0.6, 0.9, 60); c.secs = 1.2;
    }
    else if (starts ("vox_")) { p["vox_on"] = 1; p["a_wave"] = 2; p["e2_a"] = 200; p["e2_d"] = 400; p["e2_s"] = 0.2f; if (id == "vox_env") p["vox_vowel"] = 0.2f; }
    else if (starts ("seq_"))
    {   // hold one note so the 16-step loop runs
        p["seq_on"] = 1; p["e1_d"] = 90; p["e1_s"] = 0.0f; p["e1_r"] = 40;
        if (id == "seq_vary" || id == "seq_rate") p["seq_pat"] = 0x5555;
        if (id == "seq_swing") p["seq_pat"] = 0xFFFF;   // swing moves the off-beat 16ths, so they have to be on
        c.phrase.clear(); note (c.phrase, 0.0, 2.6, 48); c.secs = 3.0;
    }
    else if (id == "st_width") { p["a_uni"] = 5; p["a_detune"] = 0.3f; p["a_width"] = 1.0f; }
    else if (id == "limiter") { p["master_vol"] = 6; p["a_uni"] = 7; p["b_on"] = 1; p["b_uni"] = 7; p["sub_level"] = 1; }
    return c;
}

juce::String fmt (float v, int w = 7) { return juce::String (v, 2).paddedLeft (' ', w); }

int runAudit (const juce::File& reportFile)
{
    XoxicoaSynthProcessor proc;
    juce::String rep;
    rep << "Xoxicoa Synth parameter audit  (48 kHz, rendered through the real processor)\n"
        << "Each parameter is rendered at min / mid / max inside a context patch that makes that\n"
        << "module audible. Columns are differences between renders (dB):\n"
        << "  dRMS = loudness change, SPEC = mean spectral-shape change (1/12-oct bands),\n"
        << "  ENV = mean change of the 20 ms loudness envelope, BAND = mean change of low/mid/high band\n"
        << "  levels per 20 ms frame (tails, echoes, sweeps), SIDE = stereo side/mid change (mono clamps at -40).\n"
        << "  score = max(|dRMS|, SPEC, ENV, BAND, |SIDE|/2, |dPEAK|). PASS needs score >= 1.0 dB on BOTH halves.\n"
        << "  Choices/switches: every option is compared with its neighbour.\n\n";
    rep << juce::String ("param").paddedRight (' ', 14) << " | " << "min->mid: dRMS   SPEC    ENV   BAND   SIDE  score | mid->max: dRMS   SPEC    ENV   BAND   SIDE  score | result\n";
    rep << juce::String::repeatedString ("-", 128) << "\n";
    int fails = 0, total = 0;
    for (auto& d : xs::paramDefs())
    {
        if (! d.affectsSound) continue;
        if (auto* only = std::getenv ("XS_ONLY"))   // e.g. XS_ONLY=punch,click
            if (! juce::StringArray::fromTokens (only, ",", "").contains (d.id)) continue;
        ++total;
        auto ctx = contextFor (d.id);
        auto* param = proc.apvts.getParameter (d.id);
        std::vector<float> values;
        if (d.kind == xs::Kind::Float) values = { d.min, param->convertFrom0to1 (0.5f), d.max };
        else if (juce::String (d.id) == "seq_pat") values = { (float) 0x1111, (float) 0x5555, (float) 0xFFFF };   // 3 patterns, not all 65536
        else for (float v = d.min; v <= d.max + 0.01f; v += 1.0f) values.push_back (v);
        std::vector<Features> feats;
        for (float v : values)
        {
            auto patch = ctx.patch;
            patch[d.id] = v;
            proc.applyPatch (patch, "audit");
            feats.push_back (analyse (render (proc, ctx.phrase, ctx.secs)));
        }
        bool ok = true;
        juce::String line = juce::String (d.id).paddedRight (' ', 14) + " | ";
        if (d.kind == xs::Kind::Float)
        {
            for (int h = 0; h < 2; ++h)
            {
                const auto dd = distance (feats[(size_t) h], feats[(size_t) h + 1]);
                line << "     " << fmt (dd.rms) << fmt (dd.spec) << fmt (dd.env) << fmt (dd.band) << fmt (dd.side) << fmt (dd.score()) << " | ";
                ok = ok && dd.score() >= 1.0f;
            }
        }
        else
        {
            float minScore = 1e9f; int worst = 0;
            for (size_t i = 1; i < feats.size(); ++i)
            {
                const float s = distance (feats[i - 1], feats[i]).score();
                if (s < minScore) { minScore = s; worst = (int) i; }
            }
            const auto dd = distance (feats.front(), feats.back());
            line << "first->last: " << fmt (dd.rms) << fmt (dd.spec) << fmt (dd.env) << fmt (dd.band) << fmt (dd.side) << fmt (dd.score())
                 << " | weakest step " << param->getText (param->convertTo0to1 (values[(size_t) worst - 1]), 16) << " -> "
                 << param->getText (param->convertTo0to1 (values[(size_t) worst]), 16) << ": " << juce::String (minScore, 2) << " dB | ";
            ok = minScore >= 1.0f;
        }
        for (auto& f : feats) ok = ok && f.finite;
        line << (ok ? "PASS" : "FAIL") << "\n";
        if (! ok) ++fails;
        rep << line;
        std::printf ("%s", line.toRawUTF8());
        std::fflush (stdout);
    }
    rep << "\n" << (total - fails) << " / " << total << " parameters pass.\n";
    reportFile.replaceWithText (rep);
    std::printf ("\n%d / %d pass\n", total - fails, total);
    return fails == 0 ? 0 : 1;
}

// ---------------- safety
float hfBurstDb (const juce::AudioBuffer<float>& b, double t, double win = 0.006)
{
    // energy above ~5 kHz (2nd-difference highpass) around time t, relative to the full-band RMS of the note
    const int c = (int) (t * kFs), w = (int) (win * kFs);
    const float* L = b.getReadPointer (0);
    double hf = 0, full = 0; int cnt = 0;
    for (int i = juce::jmax (2, c - w); i < juce::jmin (b.getNumSamples(), c + w); ++i, ++cnt)
    {
        const double d2 = L[i] - 2.0 * L[i - 1] + L[i - 2];
        hf += d2 * d2;
    }
    for (int i = (int) (0.1 * kFs); i < (int) (0.3 * kFs); ++i) full += L[i] * (double) L[i];
    return (float) (10.0 * std::log10 ((hf / juce::jmax (1, cnt)) / (full / (0.2 * kFs) + 1e-20) + 1e-20));
}

int runSafety (const juce::File& reportFile)
{
    XoxicoaSynthProcessor proc;
    juce::String rep;
    int problems = 0;
    auto check = [&] (bool ok, const juce::String& what) { if (! ok) { ++problems; rep << "  PROBLEM: " << what << "\n"; } };

    // 1. clicks: pure sine, zero attack, minimum release, notes cut at awkward phases
    rep << "== Clicks (sine, attack 0, release 1 ms -> engine minimums 1 ms / 4 ms)\n";
    rep << "   HF burst = energy of the 2nd difference around the event vs. the sustained note (dB). A hard\n"
        << "   on/off step measures about +10 dB; a steady 220 Hz sine about -48 dB.\n";
    {
        auto p = auditBase();
        p["a_wave"] = 0; p["e1_a"] = 0; p["e1_r"] = 1; p["e1_s"] = 1;
        proc.applyPatch (p, "click");
        Phrase ph; note (ph, 0.0, 0.5137, 57, 1.0f);
        auto b = render (proc, ph, 0.8);
        const float on = hfBurstDb (b, 0.0), off = hfBurstDb (b, 0.5137), steady = hfBurstDb (b, 0.25);
        rep << "   note-on " << juce::String (on, 1) << " dB, note-off " << juce::String (off, 1) << " dB, steady " << juce::String (steady, 1) << " dB\n";
        check (on < -20.0f && off < -20.0f, "click on note on/off");
        // retrigger / steal: 20 fast notes on 16 voices
        Phrase st;
        for (int i = 0; i < 20; ++i) note (st, 0.02 * i, 0.6, 48 + i, 1.0f);
        p["e1_r"] = 600;
        proc.applyPatch (p, "steal");
        auto b2 = render (proc, st, 1.2);
        float worst = -200;
        for (int i = 16; i < 20; ++i) worst = juce::jmax (worst, hfBurstDb (b2, 0.02 * i, 0.003));
        rep << "   voice-steal (notes 17-20 on a 16-voice synth): worst HF burst " << juce::String (worst, 1) << " dB\n";
        check (worst < -10.0f, "click on voice steal");
        // legato 808 slide
        auto p8 = xs::factoryPresets()[16].values;
        proc.applyPatch (p8, "808");
        auto b3 = render (proc, slidePhrase(), 2.2);
        const float sl = juce::jmax (hfBurstDb (b3, 0.40, 0.004), hfBurstDb (b3, 0.90, 0.004));
        rep << "   808 legato slide transition: " << juce::String (sl, 1) << " dB\n";
    }

    // 2. presets: finite, not silent, never over ceiling
    rep << "\n== Factory presets (poly phrase or slide phrase), limiter on\n";
    for (int i = 0; i < (int) xs::factoryPresets().size(); ++i)
    {
        const auto& pr = xs::factoryPresets()[(size_t) i];
        proc.loadFactory (i);
        const bool mono = pr.values.at ("voice_mode") > 0.5f;
        const auto f = analyse (render (proc, mono ? slidePhrase() : polyPhrase(), 2.6));
        rep << "   " << juce::String (xs::categories[pr.category]).paddedRight (' ', 10) << " " << pr.name.paddedRight (' ', 18)
            << " rms " << fmt (f.rmsDb) << " dB  peak " << fmt (f.peakDb) << " dBFS\n";
        check (f.finite, pr.name + " produced NaN/Inf");
        check (f.peakDb <= -0.4f, pr.name + " exceeded the -0.5 dBFS ceiling");
        check (f.rmsDb > -40.0f, pr.name + " is too quiet");
    }

    // 3. generator: 60 rolls per category across wildness 0..1
    rep << "\n== Generator: 60 rolls per category, wildness 0..1\n";
    juce::Random rng (2026);
    for (int cat = 0; cat < xs::categories.size(); ++cat)
    {
        float lo = 1e9f, hi = -1e9f, pk = -200; int bad = 0;
        for (int r = 0; r < 60; ++r)
        {
            auto patch = xs::Generator::generate (cat, (float) r / 59.0f, rng);
            const int note = xs::Generator::exportNote (cat, rng);
            bool usable = true;
            Features f;
            if (xs::isKitType (cat))
            {   // one-shots / loops / fx / vox: same path as GENERATE, judged on the exported sound
                XoxicoaSynthProcessor::normaliseKit (patch, cat, note, 150);
                XoxicoaSynthProcessor kp;
                kp.applyPatch (patch, "gen");
                for (auto [id, v] : { std::pair<const char*, float> { "gen_cat", (float) cat }, { "exp_note", (float) note }, { "exp_bpm", 150.0f } })
                    kp.apvts.getParameter (id)->setValueNotifyingHost (kp.apvts.getParameter (id)->convertTo0to1 (v));
                f = analyse (XoxicoaSynthProcessor::renderOffline (kp, 44100.0));
                usable = f.finite && f.peakDb > -9.0f && f.peakDb <= -0.4f && f.rmsDb > -50.0f;
            }
            else
            {
                XoxicoaSynthProcessor::normaliseLoudness (patch, cat);   // same as the Generate button
                proc.applyPatch (patch, "gen");
                const bool mono = patch["voice_mode"] > 0.5f;
                f = analyse (render (proc, mono ? slidePhrase() : polyPhrase(), 2.0));
                usable = f.finite && f.rmsDb >= -40.0f && f.peakDb <= -0.4f;
            }
            if (! usable)
            {
                ++bad;
                if (std::getenv ("XS_DEBUG"))
                {
                    float pk = 0; const float pr = XoxicoaSynthProcessor::probeLoudnessDb (patch, &pk, cat);
                    std::printf ("BAD cat %d roll %d rms %.1f peak %.1f master %.1f probe %.1f/%.1f\n", cat, r, f.rmsDb, f.peakDb, patch["master_vol"], pr, pk);
                    for (auto& kv : patch) std::printf ("  %s=%.3f", kv.first.toRawUTF8(), kv.second);
                    std::printf ("\n");
                }
            }
            lo = juce::jmin (lo, f.rmsDb); hi = juce::jmax (hi, f.rmsDb); pk = juce::jmax (pk, f.peakDb);
        }
        rep << "   " << juce::String (xs::categories[cat]).paddedRight (' ', 10) << " rms range " << fmt (lo) << " .. " << fmt (hi)
            << " dB, worst peak " << fmt (pk) << " dBFS, unusable rolls: " << bad << "/60\n";
        check (bad == 0, xs::categories[cat] + " generator produced unusable rolls");
    }

    // 4. abuse: 100 patches with every parameter random over its full range
    rep << "\n== Abuse: 100 fully random patches (every param over its full range)\n";
    {
        int nan = 0, over = 0;
        for (int r = 0; r < 100; ++r)
        {
            xs::Patch p;
            for (auto& d : xs::paramDefs())
                if (d.affectsSound)
                {
                    float v = d.min + rng.nextFloat() * (d.max - d.min);
                    if (d.kind != xs::Kind::Float) v = std::round (v);
                    p[d.id] = v;
                }
            p["master_vol"] = 6; p["limiter"] = 1;
            proc.applyPatch (p, "abuse");
            Phrase ph = polyPhrase(); ph.push_back ({ 0.5, 0, 0, false, 0 });
            const auto f = analyse (render (proc, ph, 2.0));
            nan += ! f.finite; over += f.peakDb > -0.4f;
        }
        rep << "   NaN/Inf renders: " << nan << ", renders over ceiling: " << over << "\n";
        check (nan == 0, "NaN in random patches");
        check (over == 0, "limiter let a random patch over the ceiling");
    }

    // 5. CPU
    rep << "\n== CPU (single thread, this Linux box; 10 s renders)\n";
    auto cpu = [&] (const juce::String& label, const xs::Patch& p, int voices)
    {
        proc.applyPatch (p, "cpu");
        Phrase ph;
        for (int v = 0; v < voices; ++v) note (ph, 0.0, 9.0, 40 + v * 3, 0.8f);
        double secs = 0;
        render (proc, ph, 10.0, 160.0, &secs);
        const double pct = secs / 10.0 * 100.0;
        rep << "   " << label.paddedRight (' ', 52) << juce::String (pct, 2) << " % of one core\n";
        return pct;
    };
    cpu ("Glokk Siren, 1 note", xs::factoryPresets()[0].values, 1);
    cpu ("Glokk Siren, 4-note chord", xs::factoryPresets()[0].values, 4);
    cpu ("Glide 808, 1 note", xs::factoryPresets()[16].values, 1);
    auto worstP = xs::factoryPresets()[0].values;
    worstP["a_uni"] = 7; worstP["b_on"] = 1; worstP["b_uni"] = 7; worstP["sub_level"] = 0.5f; worstP["noise_level"] = 0.2f;
    worstP["flt_type"] = 1; worstP["dist_on"] = 1; worstP["cho_on"] = 1; worstP["dly_on"] = 1; worstP["rev_on"] = 1;
    const double worst = cpu ("WORST CASE: 16 voices x (7+7 unison) + sub/noise + all FX", worstP, 16);
    check (worst < 60.0, "worst-case CPU too high");

    rep << "\n" << (problems == 0 ? "ALL SAFETY CHECKS PASSED" : juce::String (problems) + " PROBLEM(S)") << "\n";
    reportFile.replaceWithText (rep);
    std::printf ("%s", rep.toRawUTF8());
    return problems == 0 ? 0 : 1;
}

// ---------------- demos
void writeWav (const juce::AudioBuffer<float>& b, const juce::File& f)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), kFs, 2, 24, {}, 0));
    if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    std::printf ("  wrote %s\n", f.getFullPathName().toRawUTF8());
}

// note helpers in beats
struct Seq
{
    double bpm; Phrase p;
    double sec (double beats) const { return beats * 60.0 / bpm; }
    void n (double b0, double len, int nn, float v = 0.85f) { note (p, sec (b0), sec (b0 + len), nn, v); }
};

Seq rageLine (int reps = 2)
{   // F minor rage riff, 160 BPM, 2 bars
    Seq s { 160.0, {} };
    const int notes[] { 65, 68, 72, 75, 72, 68, 70, 67, 65, 68, 72, 77, 75, 72, 70, 68 };
    for (int r = 0; r < reps; ++r)
        for (int i = 0; i < 16; ++i) s.n (r * 8.0 + i * 0.5, i % 4 == 3 ? 0.45 : 0.3, notes[i], i % 2 ? 0.75f : 0.95f);
    return s;
}
Seq slide808()
{   // 808 with overlapping (legato) slides, 160 BPM, 4 bars
    Seq s { 160.0, {} };
    auto n = [&s] (double b, double len, int nn) { s.n (b, len, nn, 0.95f); };
    n (0, 1.6, 41); n (1.5, 1.1, 48);          // slide up an octave-ish
    n (3.0, 0.9, 41);
    n (4, 1.6, 44); n (5.5, 1.0, 39);          // slide down
    n (6.75, 1.2, 41);
    n (8, 1.6, 41); n (9.5, 0.6, 53); n (10.0, 1.5, 41);   // octave flick
    n (12, 1.0, 46); n (13, 0.6, 44); n (13.5, 2.0, 41);
    return s;
}
Seq pluggMelody()
{   // 150 BPM
    Seq s { 150.0, {} };
    const int mel[] { 77, 80, 84, 82, 80, 77, 75, 77, 80, 77, 72, 75, 77, 75, 72, 70 };
    const double pos[] { 0, 0.75, 1.5, 2, 2.5, 3, 3.5, 4.0, 4.75, 5.5, 6, 6.5, 7, 8, 8.75, 9.5 };
    for (int i = 0; i < 16; ++i) s.n (pos[i], 0.4, mel[i], 0.8f);
    return s;
}
Seq padChords()
{
    Seq s { 150.0, {} };
    const int ch[4][4] { { 53, 56, 60, 65 }, { 49, 53, 56, 61 }, { 44, 48, 51, 56 }, { 51, 55, 58, 63 } };
    for (int c = 0; c < 4; ++c) for (int v = 0; v < 4; ++v) s.n (c * 4.0, 3.9, ch[c][v], 0.7f);
    return s;
}
Seq pluckArp()
{
    Seq s { 160.0, {} };
    const int ar[] { 65, 72, 68, 75, 72, 77, 75, 68 };
    for (int r = 0; r < 4; ++r) for (int i = 0; i < 8; ++i) s.n (r * 4.0 + i * 0.5, 0.25, ar[i] + (r == 2 ? -4 : 0), 0.85f);
    return s;
}

int runDemos (const juce::File& dir)
{
    dir.createDirectory();
    XoxicoaSynthProcessor proc;
    auto go = [&] (const xs::Patch& patch, const Seq& s, const juce::String& file, double extra = 2.0)
    {
        proc.applyPatch (patch, file);
        double last = 0; for (auto& e : s.p) last = juce::jmax (last, e.t);
        writeWav (render (proc, s.p, last + extra, s.bpm), dir.getChildFile (file));
    };
    const auto& f = xs::factoryPresets();
    auto byName = [&f] (const char* n) { for (auto& p : f) if (p.name == n) return p.values; return f[0].values; };

    // headline demos
    go (byName ("Glide 808"), slide808(), "808_slide_160bpm_Glide808.wav");
    go (byName ("Mosh 808 Dist"), slide808(), "808_slide_160bpm_Mosh808Dist.wav");
    go (byName ("Glokk Siren"), rageLine(), "rage_lead_160bpm_GlokkSiren.wav");
    go (byName ("Mosh Pit Saw"), rageLine(), "rage_lead_160bpm_MoshPitSaw.wav");
    go (byName ("Plugg Glass"), pluggMelody(), "plugg_bell_150bpm_PluggGlass.wav");
    go (byName ("Void Pad"), padChords(), "dark_pad_150bpm_VoidPad.wav", 3.0);
    go (byName ("Rage Pluck"), pluckArp(), "pluck_160bpm_RagePluck.wav");

    // generated sounds: 3 rolls per category at increasing wildness (fixed seeds so they are reproducible)
    for (int cat = 0; cat < xs::categories.size(); ++cat)
        for (int r = 0; r < 3; ++r)
        {
            juce::Random rng (1000 + cat * 10 + r);
            const float wild = 0.15f + 0.35f * (float) r;
            auto patch = xs::Generator::generate (cat, wild, rng);
            XoxicoaSynthProcessor::normaliseLoudness (patch, cat);
            const Seq s = xs::is808Type (cat) ? slide808() : cat == 0 ? rageLine (1) : (cat == 1 || xs::isLightType (cat)) ? (cat == xs::stAiryPad ? padChords() : pluggMelody())
                        : cat == 3 ? padChords() : pluckArp();
            go (patch, s, "generated_" + xs::categories[cat].removeCharacters (" ").toLowerCase() + "_roll" + juce::String (r + 1)
                              + "_wild" + juce::String (juce::roundToInt (wild * 100)) + ".wav");
        }

    // a mini beat: 808 + rage lead layered at 160 BPM (two renders summed)
    {
        proc.applyPatch (byName ("Glide 808"), "mix");
        auto bass = render (proc, slide808().p, 12.0, 160.0);
        proc.applyPatch (byName ("Glokk Siren"), "mix");
        Seq lead = rageLine (2);
        auto ld = render (proc, lead.p, 12.0, 160.0);
        for (int c = 0; c < 2; ++c) { bass.applyGain (c, 0, bass.getNumSamples(), 0.6f); bass.addFrom (c, 0, ld, c, 0, ld.getNumSamples(), 0.5f); }
        writeWav (bass, dir.getChildFile ("mix_808_plus_rage_lead_160bpm.wav"));
    }
    return 0;
}
int runCalibrate (const juce::File& header)
{
    juce::String out;
    out << "#pragma once\n// GENERATED by `XoxicoaSynthAudit calibrate` - loudness-matched master level per factory preset.\n"
        << "#include <juce_core/juce_core.h>\n#include <map>\nnamespace xs\n{\ninline const std::map<juce::String, float>& factoryMasterDb()\n{\n"
        << "    static const std::map<juce::String, float> m {\n";
    for (auto& pr : xs::factoryPresets())
    {
        auto patch = pr.values;
        XoxicoaSynthProcessor::normaliseLoudness (patch, pr.category);
        out << "        { \"" << pr.name << "\", " << juce::String (patch["master_vol"], 1) << "f },\n";
        std::printf ("%-18s %6.1f dB\n", pr.name.toRawUTF8(), patch["master_vol"]);
    }
    out << "    };\n    return m;\n}\n}\n";
    header.replaceWithText (out);
    return 0;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode = argc > 1 ? argv[1] : "audit";
    const juce::String arg = argc > 2 ? argv[2] : "";
    if (mode == "audit")  return runAudit (juce::File (arg.isEmpty() ? "audit.txt" : arg));
    if (mode == "safety") return runSafety (juce::File (arg.isEmpty() ? "safety.txt" : arg));
    if (mode == "probe")
    {
        juce::Random rng (77);
        for (int r = 0; r < (std::getenv ("XS_N") ? std::atoi (std::getenv ("XS_N")) : 40); ++r)
        {
            auto patch = xs::Generator::generate (1, 0.5f, rng);
            float pk1, pk2;
            const float a = XoxicoaSynthProcessor::probeLoudnessDb (patch, &pk1);
            const float b = XoxicoaSynthProcessor::probeLoudnessDb (patch, &pk2);
            if (std::abs (a - b) > 0.01f || std::abs (pk1 - pk2) > 0.01f) std::printf ("roll %d: %.2f/%.2f vs %.2f/%.2f\n", r, a, pk1, b, pk2);
        }
        return 0;
    }
    if (mode == "calibrate") return runCalibrate (juce::File (arg));
    if (mode == "demos")  return runDemos (juce::File (arg.isEmpty() ? "synth_demo" : arg));
    std::printf ("usage: XoxicoaSynthAudit audit|safety|demos <path>\n");
    return 2;
}
