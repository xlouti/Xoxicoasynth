// 808 / light-synth measurement + demo renderer (runs the real processor, offline).
//   XoxicoaSynthMeasure measure <report.txt> [wavDir]   808 sub / transient / harmonics / loudness
//   XoxicoaSynthMeasure pattern <out.wav> <presetName> <rage|plugg> [bars]
//   XoxicoaSynthMeasure note    <out.wav> <presetName> <midiNote> <seconds>
// This file only uses API that exists in v1.1 too, so the same tool measures "before" and "after".
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>

namespace
{
constexpr double kFs = 48000.0;
constexpr int kBlock = 256;

struct Head : juce::AudioPlayHead
{
    double bpm = 160.0; juce::int64 sample = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setBpm (bpm); p.setIsPlaying (true); p.setTimeInSamples (sample);
        p.setPpqPosition ((double) sample / kFs * bpm / 60.0); p.setTimeSignature (TimeSignature { 4, 4 });
        return p;
    }
};
struct Ev { double t; int note; float vel; bool on; };
using Phrase = std::vector<Ev>;
void note (Phrase& p, double t0, double t1, int n, float v = 0.9f) { p.push_back ({ t0, n, v, true }); p.push_back ({ t1, n, 0.0f, false }); }

juce::AudioBuffer<float> render (XoxicoaSynthProcessor& proc, Phrase ph, double secs, double bpm)
{
    Head head; head.bpm = bpm;
    proc.setPlayHead (&head);
    proc.prepareToPlay (kFs, kBlock);
    std::stable_sort (ph.begin(), ph.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
    const int lat = proc.getLatencySamples();
    const int total = (int) (secs * kFs) + lat;
    juce::AudioBuffer<float> out (2, total), blk (2, kBlock);
    size_t ei = 0;
    for (int pos = 0; pos < total; pos += kBlock)
    {
        const int n = juce::jmin (kBlock, total - pos);
        juce::MidiBuffer midi;
        while (ei < ph.size() && (int) (ph[ei].t * kFs) < pos + n)
        {
            const auto& e = ph[ei++];
            const int at = juce::jlimit (0, n - 1, (int) (e.t * kFs) - pos);
            midi.addEvent (e.on ? juce::MidiMessage::noteOn (1, e.note, e.vel) : juce::MidiMessage::noteOff (1, e.note), at);
        }
        blk.setSize (2, n, false, false, true);
        head.sample = pos;
        proc.processBlock (blk, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, blk, c, 0, n);
    }
    proc.setPlayHead (nullptr);
    juce::AudioBuffer<float> a (2, total - lat);
    for (int c = 0; c < 2; ++c) a.copyFrom (c, 0, out, c, lat, total - lat);
    return a;
}

void writeWav (const juce::AudioBuffer<float>& b, const juce::File& f)
{
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), kFs, 2, 24, {}, 0));
    if (w) { os.release(); w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); }
}

double rmsDb (const std::vector<float>& x, int a, int b)
{
    a = juce::jlimit (0, (int) x.size(), a); b = juce::jlimit (a, (int) x.size(), b);
    double s = 0; for (int i = a; i < b; ++i) s += x[(size_t) i] * (double) x[(size_t) i];
    return 10.0 * std::log10 (s / juce::jmax (1, b - a) + 1e-20);
}
std::vector<float> filt (const std::vector<float>& x, bool low, double hz)
{   // 4th order (two cascaded biquads)
    juce::IIRFilter f1, f2;
    auto c = low ? juce::IIRCoefficients::makeLowPass (kFs, hz) : juce::IIRCoefficients::makeHighPass (kFs, hz);
    f1.setCoefficients (c); f2.setCoefficients (c);
    std::vector<float> y (x.size());
    for (size_t i = 0; i < x.size(); ++i) y[i] = f2.processSingleSampleRaw (f1.processSingleSampleRaw (x[i]));
    return y;
}
// zero-crossing frequency of the low band in a window (Hz)
double zcFreq (const std::vector<float>& lo, int a, int b)
{
    int first = -1, last = -1, n = 0;
    for (int i = juce::jmax (1, a); i < juce::jmin ((int) lo.size(), b); ++i)
        if (lo[(size_t) i - 1] < 0.0f && lo[(size_t) i] >= 0.0f) { if (first < 0) first = i; last = i; ++n; }
    return n >= 2 ? (n - 1) * kFs / (double) (last - first) : 0.0;
}

struct M
{
    double f0 = 0, peak = 0, rms = 0, crest = 0, sub = 0, upper = 0, upMinusSub = 0, punch = 0, dropSt = 0, harmCount = 0, thd = 0;
    double h[9] {};   // H2..H10 relative to H1 (dB)
};

M measureNote (const juce::AudioBuffer<float>& b)
{
    const int n = b.getNumSamples();
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i) x[(size_t) i] = 0.5f * (b.getSample (0, i) + b.getSample (1, i));
    M m;
    float pk = 0; for (int i = 0; i < n; ++i) pk = juce::jmax (pk, std::abs (b.getSample (0, i)), std::abs (b.getSample (1, i)));
    m.peak = juce::Decibels::gainToDecibels (pk, -200.0f);
    const int s500 = (int) (0.5 * kFs), s800 = (int) (0.8 * kFs);
    m.rms = rmsDb (x, 0, s800);
    m.crest = m.peak - rmsDb (x, 0, s500);
    const auto lo = filt (x, true, 120.0), hi = filt (x, false, 200.0);
    m.sub = rmsDb (lo, 0, s800);
    m.upper = rmsDb (hi, 0, s800);
    m.upMinusSub = m.upper - m.sub;
    m.punch = rmsDb (x, 0, (int) (0.03 * kFs)) - rmsDb (x, (int) (0.15 * kFs), (int) (0.4 * kFs));
    // spectrum of the body (150-320 ms)
    constexpr int order = 14, N = 1 << order;
    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> win (N, juce::dsp::WindowingFunction<float>::hann);
    std::vector<float> buf (N * 2, 0.0f);
    const int s0 = (int) (0.12 * kFs);
    for (int i = 0; i < N && s0 + i < n; ++i) buf[(size_t) i] = x[(size_t) (s0 + i)];
    win.multiplyWithWindowingTable (buf.data(), N);
    fft.performFrequencyOnlyForwardTransform (buf.data());
    const double binHz = kFs / N;
    int kb = 0; float best = 0;
    for (int k = (int) (18 / binHz); k < (int) (200 / binHz); ++k) if (buf[(size_t) k] > best) { best = buf[(size_t) k]; kb = k; }
    m.f0 = kb * binHz;
    auto ampAt = [&] (double f) { float a = 0; const int k0 = (int) (f * 0.97 / binHz), k1 = (int) (f * 1.03 / binHz) + 1; for (int k = k0; k <= k1 && k < N / 2; ++k) a = juce::jmax (a, buf[(size_t) k]); return a; };
    const double a1 = ampAt (m.f0) + 1e-12;
    double hs = 0;
    for (int k = 2; k <= 10; ++k)
    {
        const double a = ampAt (m.f0 * k);
        m.h[k - 2] = 20.0 * std::log10 (a / a1 + 1e-12);
        hs += (a / a1) * (a / a1);
        if (m.h[k - 2] > -30.0) m.harmCount += 1;
    }
    m.thd = 10.0 * std::log10 (hs + 1e-12);
    // pitch drop: frequency of the first full cycle after the onset vs the body (low band, zero crossings)
    const auto lo2 = filt (x, true, 400.0);
    double fA = 0.0;
    {
        int c[2] { -1, -1 }, nc = 0;
        for (int i = 1; i < (int) (0.2 * kFs) && nc < 2; ++i)
            if (lo2[(size_t) i - 1] < 0.0f && lo2[(size_t) i] >= 0.0f) c[nc++] = i;
        if (nc == 2) fA = kFs / (double) (c[1] - c[0]);
    }
    const double fB = zcFreq (lo2, (int) (0.2 * kFs), (int) (0.45 * kFs));
    m.dropSt = (fA > 0 && fB > 0) ? 12.0 * std::log2 (fA / fB) : 0.0;
    return m;
}

const xs::Preset* findPreset (const juce::String& name)
{
    for (auto& p : xs::factoryPresets()) if (p.name.equalsIgnoreCase (name)) return &p;
    return nullptr;
}

juce::String row (const juce::String& name, const M& m)
{
    auto f = [] (double v, int w = 7, int d = 1) { return juce::String (v, d).paddedLeft (' ', w); };
    juce::String s = name.paddedRight (' ', 26);
    s << f (m.f0, 6, 1) << f (m.peak) << f (m.rms) << f (m.crest) << f (m.sub) << f (m.upper) << f (m.upMinusSub) << f (m.punch) << f (m.dropSt) << f (m.thd) << f (m.harmCount, 5, 1) << " ";
    for (int k = 0; k < 5; ++k) s << juce::String (juce::roundToInt (juce::jmax (-99.0, m.h[k]))).paddedLeft (' ', 6);
    return s + "\n";
}

int runMeasure (const juce::File& report, const juce::File& wavDir)
{
    juce::String rep;
    rep << "808 measurement (48 kHz, limiter on, master as the patch sets it). One note A1 (MIDI 33, 55 Hz), velocity 0.9,\n"
        << "held 1.0 s. Everything is measured on the mono sum.\n"
        << "  f0     = strongest spectral peak 18-200 Hz in the body (120-460 ms)\n"
        << "  peak   = sample peak dBFS        rms = RMS 0-800 ms dBFS        crest = peak - RMS(0-500 ms)\n"
        << "  sub    = RMS below 120 Hz        upper = RMS above 200 Hz (what a phone / laptop speaker can play)\n"
        << "  up-sub = upper minus sub         punch = RMS(0-30 ms) - RMS(150-400 ms)   drop = pitch of the first cycle vs body (st)\n"
        << "  THD    = H2..H10 energy vs H1 (dB)   #H = harmonics above -30 dB   H2..H6 = level vs fundamental (dB)\n\n";
    rep << juce::String ("sound").paddedRight (' ', 26) << "    f0   peak    rms  crest    sub  upper up-sub  punch   drop    THD  #H     H2    H3    H4    H5    H6\n";
    XoxicoaSynthProcessor proc;
    auto doOne = [&] (const juce::String& name, const xs::Patch& patch)
    {
        proc.applyPatch (patch, name);
        Phrase ph; note (ph, 0.0, 1.0, 33, 0.9f);
        auto b = render (proc, ph, 1.6, 160.0);
        const auto m = measureNote (b);
        rep << row (name, m);
        if (wavDir != juce::File()) writeWav (b, wavDir.getChildFile (name.replaceCharacters (" :/", "___") + ".wav"));
        return m;
    };
    rep << "-- factory 808 presets\n";
    for (auto& p : xs::factoryPresets())
        if (xs::categories[p.category].contains ("808")) doOne (p.name, p.values);
    rep << "-- generator rolls, type 808, Wild 0.35, seeds 1-12 (loudness-matched like GENERATE)\n";
    std::vector<M> rolls;
    for (int s = 1; s <= 12; ++s)
    {
        juce::Random rng (s);
        auto patch = xs::Generator::generate (xs::st808, 0.35f, rng);
        XoxicoaSynthProcessor::normaliseLoudness (patch, xs::st808);
        rolls.push_back (doOne ("roll " + juce::String (s), patch));
    }
    auto average = [] (const std::vector<M>& rolls)
    {
    M avg;
    for (auto& r : rolls) { avg.f0 += r.f0; avg.peak += r.peak; avg.rms += r.rms; avg.crest += r.crest; avg.sub += r.sub; avg.upper += r.upper; avg.upMinusSub += r.upMinusSub; avg.punch += r.punch; avg.dropSt += r.dropSt; avg.thd += r.thd; avg.harmCount += r.harmCount; for (int k = 0; k < 9; ++k) avg.h[k] += r.h[k]; }
    const double nr = (double) rolls.size();
    avg.f0 /= nr; avg.peak /= nr; avg.rms /= nr; avg.crest /= nr; avg.sub /= nr; avg.upper /= nr; avg.upMinusSub /= nr; avg.punch /= nr; avg.dropSt /= nr; avg.thd /= nr; avg.harmCount /= nr; for (auto& v : avg.h) v /= nr;
    return avg;
    };
    rep << row ("ROLL AVERAGE", average (rolls));
    // v1.2 808 types: 6 rolls each
    const std::pair<int, const char*> types[] { { xs::st808Clean, "avg 808 Clean Sub" }, { xs::st808Rage, "avg 808 Rage" }, { xs::st808Plugg, "avg 808 Pluggnb" },
                                                { xs::st808Spinz, "avg 808 Spinz" }, { xs::st808Boom, "avg 808 Long Boom" } };
    rep << "-- v1.2 generator types, Wild 0.35, seeds 1-6 each (averages)\n";
    std::vector<M> all = rolls;
    for (auto [t, label] : types)
    {
        std::vector<M> rr;
        for (int s = 1; s <= 6; ++s)
        {
            juce::Random rng (100 * t + s);
            auto patch = xs::Generator::generate (t, 0.35f, rng);
            XoxicoaSynthProcessor::normaliseLoudness (patch, t);
            proc.applyPatch (patch, "r");
            Phrase ph; note (ph, 0.0, 1.0, 33, 0.9f);
            auto b = render (proc, ph, 1.6, 160.0);
            rr.push_back (measureNote (b));
        }
        rep << row (label, average (rr));
        all.insert (all.end(), rr.begin(), rr.end());
    }
    rep << row ("ALL 42 ROLLS AVG", average (all));
    report.replaceWithText (rep);
    std::printf ("%s", rep.toRawUTF8());
    return 0;
}

// 160 BPM rage: 808 on the 1, slides into the octave and fifth; 150 BPM pluggnb: bouncier, longer slides
Phrase rage808 (int bars, double bpm, int root)
{
    Phrase p; const double b = 60.0 / bpm;
    for (int bar = 0; bar < bars; ++bar)
    {
        const double t = bar * 4 * b;
        const int r = root + (bar % 4 == 3 ? -2 : 0);
        note (p, t + 0.0 * b, t + 1.05 * b, r, 1.0f);
        note (p, t + 1.0 * b, t + 1.45 * b, r + 12, 0.95f);       // overlapping -> legato glide up
        note (p, t + 1.5 * b, t + 2.3 * b, r, 1.0f);
        note (p, t + 2.75 * b, t + 3.55 * b, r, 1.0f);
        note (p, t + 3.5 * b, t + 3.98 * b, r + 7, 0.95f);        // glide to the fifth
    }
    return p;
}
Phrase plugg808 (int bars, double bpm, int root)
{
    Phrase p; const double b = 60.0 / bpm;
    for (int bar = 0; bar < bars; ++bar)
    {
        const double t = bar * 4 * b;
        const int r = root + (bar % 2 == 1 ? 3 : 0);
        note (p, t + 0.0 * b, t + 0.8 * b, r, 1.0f);
        note (p, t + 0.75 * b, t + 1.2 * b, r + 5, 0.9f);           // slide
        note (p, t + 1.5 * b, t + 2.0 * b, r, 0.95f);
        note (p, t + 2.5 * b, t + 3.6 * b, r - 2, 1.0f);
        note (p, t + 3.5 * b, t + 3.95 * b, r + 10, 0.85f);         // long spinz glide up
    }
    return p;
}
Phrase lightPhrase (int bars, double bpm, bool chords)
{   // pluggnb progression: F#m - D - A - E (light synths), melody or chords
    Phrase p; const double b = 60.0 / bpm;
    const int roots[4] { 66, 62, 69, 64 };
    const int third[4] { 3, 4, 4, 4 };
    for (int bar = 0; bar < bars; ++bar)
    {
        const double t = bar * 4 * b; const int r = roots[bar % 4];
        if (chords) { for (int iv : { 0, third[bar % 4], 7 }) note (p, t, t + 3.8 * b, r + iv - 12, 0.75f); }
        else
        {
            const int mel[8] { 0, 7, 12, third[bar % 4] + 12, 7, 12, 14, 7 };
            for (int i = 0; i < 8; ++i) note (p, t + i * 0.5 * b, t + i * 0.5 * b + 0.42 * b, r + mel[i], i % 2 ? 0.7f : 0.85f);
        }
    }
    return p;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode = argc > 1 ? argv[1] : "";
    if (mode == "measure" && argc > 2) return runMeasure (juce::File (argv[2]), argc > 3 ? juce::File (argv[3]) : juce::File());
    if ((mode == "pattern" || mode == "light" || mode == "chords") && argc > 4)
    {
        const auto* pr = findPreset (argv[3]);
        if (! pr) { std::printf ("no preset %s\n", argv[3]); return 1; }
        XoxicoaSynthProcessor proc;
        proc.loadFactory ((int) (pr - xs::factoryPresets().data()));   // with its calibrated factory level, like picking it in the plugin
        const juce::String style (argv[4]);
        const int bars = argc > 5 ? std::atoi (argv[5]) : 4;
        const double bpm = style == "rage" ? 160.0 : 150.0;
        Phrase ph = mode == "pattern" ? (style == "rage" ? rage808 (bars, bpm, 30) : plugg808 (bars, bpm, 30))
                                      : lightPhrase (bars, bpm, mode == "chords");
        auto b = render (proc, ph, bars * 4 * 60.0 / bpm + 1.5, bpm);
        writeWav (b, juce::File (argv[2]));
        std::printf ("wrote %s\n", argv[2]);
        return 0;
    }
    if (mode == "knobcheck" && argc > 3)
    {   // knobcheck <type> <param>: generated roll (like the knob audit), param at its value and moved 35%
        const int t = std::atoi (argv[2]);
        for (int mv = 0; mv < 3; ++mv)
        {
            XoxicoaSynthProcessor p;
            auto setv = [&p] (const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
            setv ("gen_cat", (float) t);
            juce::Random r (4242 + t);
            auto patch = xs::Generator::generate (t, p.apvts.getRawParameterValue ("gen_wild")->load(), r);
            setv ("exp_note", (float) xs::Generator::exportNote (t, r));
            XoxicoaSynthProcessor::normaliseLoudness (patch, t);
            p.applyPatch (patch, "k");
            auto* par = p.apvts.getParameter (argv[3]);
            const float v = par->getValue();
            if (mv == 1) par->setValueNotifyingHost (v < 0.5f ? v + 0.35f : v - 0.35f);
            if (mv == 2) par->setValueNotifyingHost (0.0f);
            auto b = XoxicoaSynthProcessor::renderOffline (p, 44100.0);
            std::printf ("%s=%.3f  len %.2f s  peak %.2f dBFS  rms %.2f dB\n", argv[3], par->convertFrom0to1 (par->getValue()), b.getNumSamples() / 44100.0,
                         juce::Decibels::gainToDecibels (b.getMagnitude (0, b.getNumSamples())), juce::Decibels::gainToDecibels (b.getRMSLevel (0, 0, b.getNumSamples())));
        }
        return 0;
    }
    std::printf ("usage: XoxicoaSynthMeasure measure <report> [wavDir] | pattern <out.wav> <preset> <rage|plugg> [bars]\n");
    return 2;
}
