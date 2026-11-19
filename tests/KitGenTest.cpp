// Offline verification for the v1.1 sound types (drums / one-shots / loops / fx / vox).
//   XoxicoaSynthKitGenTest examples <outDir> <report.txt>   N variations per type: silence / clipping / variety, one example WAV per type
//   XoxicoaSynthKitGenTest knobs <report.txt>               every sound knob, every type: does it change the render?
//   XoxicoaSynthKitGenTest shot <out.png> <type> [seed]     UI snapshot after GENERATE on a type
//   XoxicoaSynthKitGenTest export <type> <count>            run the real Save WAV / Generate 10 code paths
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
const std::vector<int> kTypes { xs::st808, xs::stKick, xs::stSnare, xs::stClap, xs::stHiHat, xs::stOpenHat, xs::stCrash,
                                xs::stPerc, xs::stSnap, xs::stSubBass, xs::stPercLoop, xs::stFX, xs::stVox,
                                // v1.2
                                xs::st808Clean, xs::st808Rage, xs::st808Plugg, xs::st808Spinz, xs::st808Boom,
                                xs::stSoftBell, xs::stSoftPluck, xs::stAiryPad, xs::stFlute, xs::stPluggKeys, xs::stMusicBox, xs::stGlassLead };

void setP (XoxicoaSynthProcessor& p, const char* id, float v)
{
    auto* q = p.apvts.getParameter (id);
    q->setValueNotifyingHost (q->convertTo0to1 (v));
}
float getP (XoxicoaSynthProcessor& p, const juce::String& id) { return p.apvts.getRawParameterValue (id)->load(); }

void generateSeeded (XoxicoaSynthProcessor& p, int type, juce::int64 seed)
{
    setP (p, "gen_cat", (float) type);
    juce::Random r (seed);
    auto patch = xs::Generator::generate (type, getP (p, "gen_wild"), r);
    setP (p, "exp_note", (float) xs::Generator::exportNote (type, r));
    if (xs::isKitType (type)) XoxicoaSynthProcessor::normaliseKit (patch, type, (int) getP (p, "exp_note"), (int) getP (p, "exp_bpm"));
    else XoxicoaSynthProcessor::normaliseLoudness (patch, type);
    p.applyPatch (patch, "Gen: " + xs::categories[type] + " #" + juce::String (100 + (int) (seed % 900)));
}

struct Stats { float peak = 0, rmsDb = -200, secs = 0; int clipped = 0; };
Stats stats (const juce::AudioBuffer<float>& b)
{
    Stats s;
    double e = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (c, i);
            s.peak = juce::jmax (s.peak, std::abs (x));
            s.clipped += std::abs (x) >= 0.999f;
            e += (double) x * x;
        }
    s.rmsDb = (float) (10.0 * std::log10 (e / juce::jmax (1, b.getNumSamples() * b.getNumChannels()) + 1e-20));
    s.secs = (float) b.getNumSamples() / 44100.0f;
    return s;
}

// 16 log bands x 2048-sample frames, dB
std::vector<float> bands (const juce::AudioBuffer<float>& b, bool side = false)
{
    constexpr int order = 10, N = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> out;
    std::vector<float> buf (2 * N);
    const int frames = juce::jmax (1, (b.getNumSamples() + N - 1) / N);
    for (int f = 0; f < frames; ++f)
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        for (int i = 0; i < N; ++i)
        {
            const int s = f * N + i;
            if (s < b.getNumSamples()) buf[(size_t) i] = 0.5f * (b.getSample (0, s) + (side ? -1.0f : 1.0f) * b.getSample (1, s)) * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
        }
        fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int k = 0; k < 16; ++k)
        {
            const int lo = (int) (1 * std::pow (2.0, k * 9.0 / 16.0)), hi = juce::jmax (lo + 1, (int) (1 * std::pow (2.0, (k + 1) * 9.0 / 16.0)));
            double e = 0;
            for (int i = lo; i < juce::jmin (hi, N / 2); ++i) e += buf[(size_t) i] * buf[(size_t) i];
            out.push_back ((float) (10.0 * std::log10 (e + 1e-12)));
        }
    }
    return out;
}

// Mean |dB| change over time-frequency cells (cells below -70 dB of the loudest are floored).
// worstFrame (optional) = the largest mean change of any single 46 ms frame (catches transient-only changes).
float spectralDiffOne (std::vector<float> A, std::vector<float> B, float* worstFrame)
{
    const size_t n = juce::jmax (A.size(), B.size());
    A.resize (n, -120.0f); B.resize (n, -120.0f);
    float mx = -200;
    for (size_t i = 0; i < n; ++i) mx = juce::jmax (mx, A[i], B[i]);
    double d = 0; int cnt = 0;
    float worst = 0.0f;
    for (size_t f = 0; f < n / 16; ++f)
    {
        double fd = 0; int fc = 0;
        for (size_t j = f * 16; j < f * 16 + 16; ++j)
        {
            const float x = juce::jmax (A[j], mx - 70.0f), y = juce::jmax (B[j], mx - 70.0f);
            if (x > mx - 70.0f || y > mx - 70.0f) { d += std::abs (x - y); ++cnt; fd += std::abs (x - y); ++fc; }
        }
        if (fc >= 4) worst = juce::jmax (worst, (float) (fd / fc));
    }
    if (worstFrame) *worstFrame = worst;
    return cnt ? (float) (d / cnt) : 0.0f;
}

// mid + side (so stereo knobs such as Width count), audible score = max (mean, worst frame / 3)
float spectralDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float wm = 0, ws = 0;
    const float m = spectralDiffOne (bands (a), bands (b), &wm);
    // side channel only matters relative to the mid level: floor it 70 dB under the loudest mid cell
    auto SA = bands (a, true), SB = bands (b, true), MA = bands (a);
    float mx = -200; for (auto v : MA) mx = juce::jmax (mx, v);
    for (auto& v : SA) v = juce::jmax (v, mx - 70.0f);
    for (auto& v : SB) v = juce::jmax (v, mx - 70.0f);
    SA.push_back (mx); SB.push_back (mx);   // same reference level
    const float sd = spectralDiffOne (SA, SB, &ws);
    return juce::jmax (m, wm / 3.0f, sd * 0.5f, ws / 6.0f);
}

bool writeWav (const juce::AudioBuffer<float>& b, const juce::File& f)
{
    f.deleteFile();
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), 44100.0, 2, 24, {}, 0));
    if (! w) return false;
    os.release();
    return w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

int runExamples (const juce::File& dir, const juce::File& report)
{
    dir.createDirectory();
    juce::String rep;
    rep << "Xoxicoa Synth v1.1 sound types - generator check (8 rolls per type, Save-WAV render path)\n\n";
    int fails = 0;
    for (int t : kTypes)
    {
        rep << juce::String (xs::categories[t]).paddedRight (' ', 10) << "\n";
        std::vector<std::vector<float>> specs;
        for (int k = 0; k < 8; ++k)
        {
            XoxicoaSynthProcessor p;
            generateSeeded (p, t, 1000 * t + k + 7);
            const auto& raw = p.renderCurrent();       // what Save WAV writes
            XoxicoaSynthProcessor q;                   // un-normalised render for the clipping check
            juce::MemoryBlock mb; p.getStateInformation (mb); q.setStateInformation (mb.getData(), (int) mb.getSize());
            auto un = XoxicoaSynthProcessor::renderOffline (q, 44100.0);
            const auto s = stats (raw), su = stats (un);
            const bool silent = s.rmsDb < -45.0f || su.peak < 0.01f;
            const bool clip = su.clipped > 0;
            fails += silent || clip;
            rep << "   roll " << k << ": " << juce::String (s.secs, 2) << " s, peak(pre-norm) " << juce::String (juce::Decibels::gainToDecibels (su.peak), 1)
                << " dBFS, rms " << juce::String (s.rmsDb, 1) << " dB, samples >= 0 dBFS: " << su.clipped
                << ", note " << juce::MidiMessage::getMidiNoteName (p.exportNote(), true, true, 4)
                << (silent ? "  <-- SILENT" : "") << (clip ? "  <-- CLIP" : "") << "\n";
            specs.push_back (bands (raw));
            if (k == 0)
            {
                juce::String name = "Xoxicoa " + XoxicoaSynthProcessor::labelFor (t) + " 01";
                if (getP (p, "seq_on") > 0.5f) name << " " << p.exportBpm() << " BPM";
                else if (xs::isTonalType (t)) name << " (" << juce::MidiMessage::getMidiNoteName (p.exportNote(), true, false, 4) << ")";
                writeWav (raw, dir.getChildFile (name + ".wav"));
            }
            if (k < 4)
            {   // extra variations for listening
                auto sub = dir.getChildFile ("variations");
                sub.createDirectory();
                writeWav (raw, sub.getChildFile (XoxicoaSynthProcessor::labelFor (t) + " v" + juce::String (k + 1) + ".wav"));
            }
        }
        // variety: average spectral distance between consecutive rolls
        // (rendered buffers are not kept, so recompute on spectra directly)
        double var = 0; int vc = 0;
        for (size_t i = 1; i < specs.size(); ++i)
        {
            const size_t n = juce::jmin (specs[i].size(), specs[i - 1].size());
            double d = 0; for (size_t j = 0; j < n; ++j) d += std::abs (specs[i][j] - specs[i - 1][j]);
            var += n ? d / n : 0; ++vc;
        }
        rep << "   variety between rolls: " << juce::String (var / juce::jmax (1, vc), 1) << " dB mean spectral distance\n\n";
    }
    rep << (fails == 0 ? "ALL PASS (no silent rolls, no samples at/above 0 dBFS)\n" : juce::String (fails) + " FAILURES\n");
    report.replaceWithText (rep);
    std::printf ("%s", rep.toRawUTF8());
    return fails == 0 ? 0 : 1;
}

// knob -> the module switch / companion setting that must be on for it to be heard
void enableModuleFor (XoxicoaSynthProcessor& p, const juce::String& id)
{
    auto on = [&p] (const char* t) { setP (p, t, 1.0f); };
    if (id.startsWith ("a_")) on ("a_on");
    if ((id == "a_detune" || id == "a_width") && getP (p, "a_uni") < 3) setP (p, "a_uni", 3);
    if ((id == "b_detune" || id == "b_width") && getP (p, "b_uni") < 3) setP (p, "b_uni", 3);
    if (id.startsWith ("b_") || id == "ring") { on ("b_on"); if (getP (p, "b_level") < 0.2f) setP (p, "b_level", 0.5f); }
    if (id.startsWith ("dist_")) on ("dist_on");
    if (id.startsWith ("cho_")) on ("cho_on");
    if (id.startsWith ("dly_")) on ("dly_on");
    if (id.startsWith ("rev_")) on ("rev_on");
    if (id.startsWith ("vox_")) on ("vox_on");
    if (id.startsWith ("seq_")) on ("seq_on");
    if (id == "seq_swing" && ((int) getP (p, "seq_pat") & 0xAAAA) == 0) setP (p, "seq_pat", 0xFFFF);   // swing moves the off-beat steps
    if (id == "vox_env" && getP (p, "vox_vowel") < 2.0f) setP (p, "vox_vowel", 2.0f);                // room to move both ways
    if (id.startsWith ("lfo_") && id != "lfo_amt" && getP (p, "lfo_amt") < 0.2f) setP (p, "lfo_amt", 0.4f);
    if (id == "lfo_div") on ("lfo_sync");
    if (id == "lfo_rate") setP (p, "lfo_sync", 0.0f);
    if (id == "burst_gap" && getP (p, "burst") < 1) setP (p, "burst", 2);
    if ((id == "noise_color" || id == "noise_decay") && getP (p, "noise_level") < 0.2f) setP (p, "noise_level", 0.4f);
    if ((id == "sub_shape" || id == "sub_oct") && getP (p, "sub_level") < 0.2f) setP (p, "sub_level", 0.4f);
    if (id.startsWith ("e2_"))
    {   // Env 2 is heard through the filter: give it some amount, room under the cutoff and sustain to release from
        if (std::abs (getP (p, "flt_env")) < 0.2f) setP (p, "flt_env", 0.5f);
        if (getP (p, "flt_cutoff") > 4000) setP (p, "flt_cutoff", 1500);
        if (id == "e2_r" && getP (p, "e2_s") < 0.4f) setP (p, "e2_s", 0.6f);
        if (id == "e2_d" && getP (p, "e2_s") > 0.8f) setP (p, "e2_s", 0.3f);
    }
    if (id == "e1_r" && getP (p, "e1_s") < 0.3f) setP (p, "e1_s", 0.5f);                               // release needs a sustain level
    if ((id == "flt_res" || id == "flt_key" || id == "flt_env") && getP (p, "flt_cutoff") > 12000) setP (p, "flt_cutoff", 2500);
    if (id == "penv_decay" && std::abs (getP (p, "penv_amt")) < 2) setP (p, "penv_amt", 12);
    if (id == "e1_s" && getP (p, "one_shot") > 0.5f && getP (p, "e1_d") > 1500) setP (p, "e1_d", 400);
    if (id == "vox_env" && getP (p, "e2_s") < 0.3f) setP (p, "e2_s", 0.7f);
    if (id == "lfo_fade") { setP (p, "lfo_dest", 0.0f); setP (p, "lfo_sync", 0.0f); setP (p, "lfo_rate", 5.0f); if (getP (p, "lfo_amt") < 0.2f) setP (p, "lfo_amt", 0.4f); }   // a vibrato to fade in
    if (id == "low_keep") { on ("dist_on"); if (getP (p, "dist_drive") < 0.5f) setP (p, "dist_drive", 0.6f); }
    if (id == "st_width" && getP (p, "a_uni") < 3) { setP (p, "a_uni", 3); setP (p, "a_width", 1.0f); }   // width needs some stereo to widen
}

int runKnobs (const juce::File& report)
{
    const juce::StringArray midiOnly { "glide", "bend", "vel_sens", "voice_mode", "master_vol", "limiter", "one_shot" };
    juce::String rep;
    rep << "Xoxicoa Synth v1.2 - knob audit on generated sounds (44.1 kHz offline renders, deterministic)\n"
        << "Each knob is moved 35% of its travel (or toggled) from the generated value.\n"
        << "diff = max of: mean |dB| change over a 16-band x 23 ms spectrogram, worst single 23 ms frame / 3, and the same on the\n"
        << "stereo side channel (so Width counts). Level changes count too. >= 1.0 dB = audible.\n"
        << "'as rolled' = on the sound exactly as GENERATE made it; 'module on' = with that knob's section switched on\n"
        << "(e.g. Osc B knobs with Osc B on, LFO knobs with some LFO amount, Clap Gap with Claps > 0).\n"
        << "Skipped (need live MIDI, not a single note): glide, bend, velocity, voice mode, one shot (note-length behaviour);\n"
        << "master/limiter are output level.\n\n";
    int hardFails = 0;
    std::map<juce::String, int> failCount;
    for (int t : kTypes)
    {
        XoxicoaSynthProcessor base;
        generateSeeded (base, t, 4242 + t);
        juce::MemoryBlock st; base.getStateInformation (st);
        auto rend = [&st] (const juce::String& id, bool module, bool move) -> juce::AudioBuffer<float>
        {
            XoxicoaSynthProcessor q;
            q.setStateInformation (st.getData(), (int) st.getSize());
            if (module) enableModuleFor (q, id);
            if (move)
            {
                auto* par = q.apvts.getParameter (id);
                const float v = par->getValue();
                const auto* d = xs::findDef (id);
                float nv = d->kind == xs::Kind::Bool ? (v > 0.5f ? 0.0f : 1.0f) : (v < 0.5f ? v + 0.35f : v - 0.35f);
                if (d->kind == xs::Kind::Choice || d->kind == xs::Kind::Int)
                {   // make sure a stepped param really moves at least one step
                    const int steps = juce::jmax (2, par->getNumSteps());
                    if (juce::roundToInt (nv * (steps - 1)) == juce::roundToInt (v * (steps - 1))) nv = v < 0.5f ? v + 1.0f / (steps - 1) : v - 1.0f / (steps - 1);
                }
                par->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, nv));
            }
            return XoxicoaSynthProcessor::renderOffline (q, 44100.0);
        };
        const auto ref = rend ("", false, false);
        const float self = spectralDiff (ref, rend ("", false, false));
        int direct = 0, withMod = 0, total = 0;
        juce::StringArray dead;
        juce::String rows;
        for (auto& d : xs::paramDefs())
        {
            const juce::String id (d.id);
            if (! d.affectsSound || midiOnly.contains (id) || id.endsWith ("_on")) continue;
            ++total;
            const float dd = spectralDiff (ref, rend (id, false, true));
            float dm = dd;
            if (dd < 1.0f)
            {
                const auto refM = rend (id, true, false);
                dm = spectralDiff (refM, rend (id, true, true));
            }
            direct += dd >= 1.0f; withMod += dm >= 1.0f;
            if (dm < 1.0f) { dead.add (id + " (" + juce::String (dm, 2) + ")"); failCount[id]++; }
            rows << "      " << id.paddedRight (' ', 12) << " as rolled " << juce::String (dd, 2).paddedLeft (' ', 6)
                 << " dB   module on " << juce::String (dm, 2).paddedLeft (' ', 6) << " dB" << (dm < 1.0f ? "   <-- no audible change" : "") << "\n";
        }
        rep << xs::categories[t] << ": " << direct << "/" << total << " knobs audible as rolled, " << withMod << "/" << total
            << " with their section on (repeat-render noise " << juce::String (self, 3) << " dB)\n";
        if (dead.size()) rep << "   not audible: " << dead.joinIntoString (", ") << "\n";
        rep << rows << "\n";
        hardFails += dead.size();
    }
    rep << "\nSummary: " << hardFails << " type/knob combinations without an audible change\n";
    for (auto& kv : failCount) rep << "   " << kv.first << ": " << kv.second << " of " << (int) kTypes.size() << " types\n";
    report.replaceWithText (rep);
    std::printf ("%s", rep.toRawUTF8());
    return 0;
}

int runShot (const juce::File& out, int type, juce::int64 seed)
{
    XoxicoaSynthProcessor proc;
    generateSeeded (proc, type, seed);
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    m.addEvent (juce::MidiMessage::noteOn (1, proc.exportNote(), (juce::uint8) 110), 0);
    for (int blk = 0; blk < 12; ++blk) { proc.processBlock (b, m); m.clear(); }
    proc.keyboardState.noteOn (1, proc.exportNote(), 1.0f);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setVisible (true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (600);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat png;
    const bool ok = os.openedOk() && png.writeImageToStream (img, os);
    std::printf ("%s %s (%dx%d)\n", ok ? "wrote" : "FAILED", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    ed.reset();
    return ok ? 0 : 1;
}

int runExport (int type, int count)
{
    XoxicoaSynthProcessor proc;
    setP (proc, "gen_cat", (float) type);
    proc.generate();
    auto f = proc.saveWav();
    std::printf ("Save WAV -> %s\n", f.getFullPathName().toRawUTF8());
    auto again = proc.saveWav();
    std::printf ("Save WAV again (unchanged sound) -> %s (%s)\n", again.getFileName().toRawUTF8(), again == f ? "re-used" : "NEW");
    auto files = proc.generateBatch (count);
    for (auto& x : files)
    {
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatReader> r (wav.createReaderFor (x.createInputStream().release(), true));
        std::printf ("  %s  %d Hz %d-bit %.2fs\n", x.getFileName().toRawUTF8(), r ? (int) r->sampleRate : 0, r ? (int) r->bitsPerSample : 0,
                     r ? (double) r->lengthInSamples / r->sampleRate : 0.0);
    }
    return files.size() == count ? 0 : 1;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode (argc > 1 ? argv[1] : "");
    if (mode == "examples" && argc > 3) return runExamples (juce::File (argv[2]), juce::File (argv[3]));
    if (mode == "knobs" && argc > 2) return runKnobs (juce::File (argv[2]));
    if (mode == "shot" && argc > 3) return runShot (juce::File (argv[2]), std::atoi (argv[3]), argc > 4 ? std::atoll (argv[4]) : 7);
    if (mode == "export" && argc > 3) return runExport (std::atoi (argv[2]), std::atoi (argv[3]));
    std::printf ("usage: examples <dir> <report> | knobs <report> | shot <png> <type> [seed] | export <type> <count>\n");
    return 2;
}
