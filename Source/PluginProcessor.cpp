#include "PluginProcessor.h"
#include "PluginEditor.h"

XoxicoaSynthProcessor::XoxicoaSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "XoxicoaSynth", xs::createLayout())
{
    engine.bind (apvts);
    (void) xs::WavetableBank::get();   // build tables up-front, not on the audio thread
    loadFactory (0);
}

bool XoxicoaSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void XoxicoaSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, juce::jmax (samplesPerBlock, 32));
    setLatencySamples (engine.getLatency());
    keyboardState.reset();
}

void XoxicoaSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int num = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, num, true);

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            engine.setBpm (pos->getBpm().orFallback (150.0), pos->getIsPlaying(), pos->getPpqPosition().orFallback (0.0));

    // split the block at MIDI events for sample-accurate notes
    int cursor = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, num, meta.samplePosition);
        if (at > cursor) { engine.render (buffer, cursor, at - cursor); cursor = at; }
        engine.handleMidi (meta.getMessage());
    }
    if (cursor < num) engine.render (buffer, cursor, num - cursor);
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, num);
}

const juce::String XoxicoaSynthProcessor::getProgramName (int index)
{
    const auto& f = xs::factoryPresets();
    return juce::isPositiveAndBelow (index, (int) f.size()) ? f[(size_t) index].name : juce::String();
}

void XoxicoaSynthProcessor::applyPatch (const xs::Patch& patch, const juce::String& name)
{
    const auto base = xs::initPatch();
    for (auto& d : xs::paramDefs())
    {
        if (! d.affectsSound) continue;
        auto* p = apvts.getParameter (d.id);
        auto it = patch.find (d.id);
        const float v = it != patch.end() ? it->second : base.at (d.id);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
    presetName = name;
    if (onPresetChanged) onPresetChanged();
}

void XoxicoaSynthProcessor::loadFactory (int index)
{
    const auto& f = xs::factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) f.size())) return;
    factoryIndex = index;
    applyPatch (f[(size_t) index].values, f[(size_t) index].name);
    if (auto* c = apvts.getParameter ("gen_cat"))
        c->setValueNotifyingHost (c->convertTo0to1 ((float) f[(size_t) index].category));
}

void XoxicoaSynthProcessor::generate()
{
    const int cat = (int) apvts.getRawParameterValue ("gen_cat")->load();
    const float wild = apvts.getRawParameterValue ("gen_wild")->load();
    genRng.setSeedRandomly();
    auto patch = xs::Generator::generate (cat, wild, genRng);
    if (auto* n = apvts.getParameter ("exp_note"))
        n->setValueNotifyingHost (n->convertTo0to1 ((float) xs::Generator::exportNote (cat, genRng)));
    if (cat >= xs::stKick)
        normaliseKit (patch, cat, exportNote(), exportBpm());
    else
        normaliseLoudness (patch, cat);
    factoryIndex = -1;
    applyPatch (patch, "Gen: " + xs::categories[cat] + " #" + juce::String (genRng.nextInt ({ 100, 999 })));
}

// One-shots / loops / fx / vox: measure the real render and land the peak on -1 dBFS before the limiter.
// Quiet hits get Boost first (pre-FX, so it also pushes the distortion), then a second measurement sets Master.
void XoxicoaSynthProcessor::normaliseKit (xs::Patch& patch, int cat, int note, int bpm)
{
    auto measure = [&] (const xs::Patch& pt)
    {
        XoxicoaSynthProcessor probe;
        auto pp = pt;
        pp["master_vol"] = 0.0f; pp["limiter"] = 0.0f;
        probe.applyPatch (pp, "probe");
        auto setv = [&probe] (const char* id, float v) { auto* q = probe.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
        setv ("gen_cat", (float) cat); setv ("exp_note", (float) note); setv ("exp_bpm", (float) bpm);
        const auto buf = renderOffline (probe, 44100.0);
        const float pk = buf.getNumSamples() > 0 ? buf.getMagnitude (0, buf.getNumSamples()) : 0.0f;
        return -1.0f - juce::Decibels::gainToDecibels (pk, -100.0f);
    };
    patch["boost"] = 0.0f;
    const float g1 = measure (patch);
    if (g1 > 6.0f) patch["boost"] = juce::jlimit (0.0f, 24.0f, g1 - 3.0f);
    const float g2 = patch["boost"] > 0.0f ? measure (patch) : g1;
    patch["master_vol"] = juce::jlimit (-24.0f, 12.0f, g2);
}

// ------------------------------------------------------------------ export
double XoxicoaSynthProcessor::holdSeconds() const
{
    auto v = [this] (const char* id) { return (double) apvts.getRawParameterValue (id)->load(); };
    if (v ("seq_on") > 0.5)
        return 16.0 * 60.0 / (double) exportBpm() * xs::seqRateBeats[juce::jlimit (0, 3, (int) v ("seq_rate"))];
    const int t = currentType();
    const double ad = (v ("e1_a") + v ("e1_d")) / 1000.0;
    if (v ("one_shot") > 0.5) return juce::jlimit (0.15, 6.0, ad);      // the voice releases by itself
    if (t == xs::st808 || t == xs::stSubBass) return juce::jlimit (0.6, 2.5, v ("e1_a") / 1000.0 + v ("e1_d") / 1000.0 * 0.9);
    if (t == xs::stDarkPad) return juce::jlimit (1.0, 4.0, v ("e1_a") / 1000.0 + 1.5);
    return juce::jlimit (0.3, 3.0, v ("e1_a") / 1000.0 + 0.8);
}

juce::AudioBuffer<float> XoxicoaSynthProcessor::renderOffline (XoxicoaSynthProcessor& p, double fs)
{
    struct Head : juce::AudioPlayHead
    {
        double bpm = 150.0;
        juce::Optional<PositionInfo> getPosition() const override { PositionInfo i; i.setBpm (bpm); i.setIsPlaying (false); return i; }
    } head;
    head.bpm = (double) p.exportBpm();
    p.setPlayHead (&head);
    constexpr int bs = 512;
    p.prepareToPlay (fs, bs);
    const int lat = p.getLatencySamples();
    const bool loop = p.apvts.getRawParameterValue ("seq_on")->load() > 0.5f;
    const double hold = p.holdSeconds();
    const int holdN = (int) std::round (hold * fs);
    const int maxN = holdN + (int) (fs * (loop ? 4.0 : 7.0)) + lat;
    const int note = p.exportNote();
    std::vector<float> L, R;
    L.reserve ((size_t) maxN); R.reserve ((size_t) maxN);
    juce::AudioBuffer<float> blk (2, bs);
    int quiet = 0;
    for (int pos = 0; pos < maxN; pos += bs)
    {
        juce::MidiBuffer m;
        if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (1, note, 1.0f), 0);
        if (holdN >= pos && holdN < pos + bs) m.addEvent (juce::MidiMessage::noteOff (1, note), holdN - pos);
        blk.clear();
        p.processBlock (blk, m);
        float pk = 0.0f;
        for (int i = 0; i < bs; ++i)
        {
            L.push_back (blk.getSample (0, i)); R.push_back (blk.getSample (1, i));
            pk = juce::jmax (pk, std::abs (blk.getSample (0, i)), std::abs (blk.getSample (1, i)));
        }
        quiet = pk < 1.0e-4f ? quiet + bs : 0;
        if (pos > holdN + lat && quiet > (int) (0.25 * fs)) break;
    }
    p.setPlayHead (nullptr);
    // drop the limiter look-ahead
    L.erase (L.begin(), L.begin() + juce::jmin ((int) L.size(), lat));
    R.erase (R.begin(), R.begin() + juce::jmin ((int) R.size(), lat));
    int len = (int) L.size();
    if (loop)
    {   // wrap the tail onto the start so the loop is seamless and exactly 16 steps long
        const int loopN = juce::jmax (1, holdN);
        for (int i = loopN; i < len; ++i) { L[(size_t) (i % loopN)] += L[(size_t) i]; R[(size_t) (i % loopN)] += R[(size_t) i]; }
        len = juce::jmin (len, loopN);
        L.resize ((size_t) loopN, 0.0f); R.resize ((size_t) loopN, 0.0f);
        len = loopN;
    }
    else
    {   // trim the silent tail (-66 dB) and fade the last 4 ms
        const float thr = 5.0e-4f;
        int end = len;
        while (end > 1 && std::abs (L[(size_t) end - 1]) < thr && std::abs (R[(size_t) end - 1]) < thr) --end;
        len = juce::jmin (len, end + (int) (0.01 * fs));
        const int fade = juce::jmin (len, (int) (0.004 * fs));
        for (int i = 0; i < fade; ++i)
        {
            const float g = (float) i / (float) fade;
            L[(size_t) (len - 1 - i)] *= g; R[(size_t) (len - 1 - i)] *= g;
        }
    }
    juce::AudioBuffer<float> out (2, juce::jmax (1, len));
    out.clear();
    for (int i = 0; i < len; ++i) { out.setSample (0, i, L[(size_t) i]); out.setSample (1, i, R[(size_t) i]); }
    return out;
}

juce::int64 XoxicoaSynthProcessor::soundHash() const
{
    juce::String s;
    for (auto& d : xs::paramDefs())
        if (d.affectsSound || juce::String (d.id).startsWith ("exp_") || juce::String (d.id) == "gen_cat")
            s << d.id << '=' << apvts.getRawParameterValue (d.id)->load() << ';';
    return s.hashCode64();
}

const juce::AudioBuffer<float>& XoxicoaSynthProcessor::renderCurrent()
{
    const auto h = soundHash();
    if (h == cachedHash && cachedRender.getNumSamples() > 0) return cachedRender;
    XoxicoaSynthProcessor tmp;
    juce::MemoryBlock mb;
    getStateInformation (mb);
    tmp.setStateInformation (mb.getData(), (int) mb.getSize());
    tmp.onPresetChanged = nullptr;
    cachedRender = renderOffline (tmp, 44100.0);
    const float pk = cachedRender.getMagnitude (0, cachedRender.getNumSamples());
    if (pk > 1.0e-6f) cachedRender.applyGain (juce::Decibels::decibelsToGain (-0.8f) / pk);
    cachedHash = h;
    return cachedRender;
}

juce::File XoxicoaSynthProcessor::kitRoot()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Xoxicoa Kit Gen");
}

juce::String XoxicoaSynthProcessor::folderFor (int t)
{
    static const juce::StringArray f { "Synth One Shots", "Synth One Shots", "808s", "Synth One Shots", "Synth One Shots",
                                       "Kicks", "Snares", "Claps", "Hi Hats", "Open Hats", "Crashes", "Percs", "Snaps",
                                       "Sub Basses", "Foley and Percussion Loops", "FX", "Vox Chants" };
    return f[juce::jlimit (0, f.size() - 1, t)];
}

juce::String XoxicoaSynthProcessor::labelFor (int t)
{
    static const juce::StringArray l { "Rage Lead", "Plugg Bell", "808", "Dark Pad", "Pluck", "Kick", "Snare", "Clap",
                                       "Hi Hat", "Open Hat", "Crash", "Perc", "Snap", "Sub", "Perc Loop", "FX", "Vox Chant" };
    return l[juce::jlimit (0, l.size() - 1, t)];
}

juce::File XoxicoaSynthProcessor::saveWav()
{
    lastExportError.clear();
    const auto h = soundHash();
    if (h == savedHash && savedFile.existsAsFile()) return savedFile;
    const int t = currentType();
    const auto dir = kitRoot().getChildFile (folderFor (t));
    if (! dir.createDirectory()) { lastExportError = "Can't create " + dir.getFullPathName(); return {}; }
    const juce::String stem = "Xoxicoa " + labelFor (t) + " ";
    int next = 1;
    for (auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.wav"))
    {
        const auto n = f.getFileNameWithoutExtension();
        if (n.startsWith (stem)) next = juce::jmax (next, n.substring (stem.length()).getIntValue() + 1);
    }
    juce::String name = stem + juce::String (next).paddedLeft ('0', 2);
    if (apvts.getRawParameterValue ("seq_on")->load() > 0.5f) name << " " << exportBpm() << " BPM";
    else if (xs::isTonalType (t)) name << " (" << juce::MidiMessage::getMidiNoteName (exportNote(), true, false, 4) << ")";
    const auto file = dir.getChildFile (name + ".wav");
    const auto& buf = renderCurrent();
    std::unique_ptr<juce::OutputStream> os (file.createOutputStream());
    if (! os) { lastExportError = "Can't write " + file.getFullPathName(); return {}; }
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), 44100.0, 2, 24, {}, 0));
    if (! w) { lastExportError = "WAV writer failed"; return {}; }
    os.release();
    w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
    w.reset();
    savedHash = h;
    savedFile = file;
    return file;
}

juce::Array<juce::File> XoxicoaSynthProcessor::generateBatch (int n)
{
    juce::Array<juce::File> files;
    for (int i = 0; i < n; ++i)
    {
        generate();
        auto f = saveWav();
        if (f == juce::File()) break;
        files.add (f);
    }
    return files;
}

float XoxicoaSynthProcessor::probeLoudnessDb (const xs::Patch& patchIn, float* peakDb)
{
    XoxicoaSynthProcessor probe;
    auto patch = patchIn;
    patch["master_vol"] = 0.0f;
    patch["limiter"] = 0.0f;
    probe.applyPatch (patch, "probe");
    constexpr double fs = 48000.0;
    constexpr int blockSize = 512;
    probe.prepareToPlay (fs, blockSize);
    const bool mono = patch["voice_mode"] > 0.5f;
    const double secs = juce::jlimit (1.2, 4.0, patch["e1_a"] / 1000.0 + 0.9);
    const int total = (int) (secs * fs);
    juce::AudioBuffer<float> buf (2, blockSize);
    const int win = (int) (0.4 * fs);
    std::vector<double> energy;
    energy.reserve ((size_t) total);
    float peak = 0.0f;
    for (int pos = 0; pos < total; pos += blockSize)
    {
        juce::MidiBuffer midi;
        if (pos == 0)
        {
            if (mono) midi.addEvent (juce::MidiMessage::noteOn (1, 36, 0.9f), 0);
            else for (int n : { 48, 55, 63 }) midi.addEvent (juce::MidiMessage::noteOn (1, n, 0.9f), 0);
        }
        if (mono && pos <= (int) (0.5 * fs) && pos + blockSize > (int) (0.5 * fs))
        {   // a legato slide, like real 808 use
            midi.addEvent (juce::MidiMessage::noteOn (1, 43, 0.9f), 0);
            midi.addEvent (juce::MidiMessage::noteOff (1, 36), 1);
        }
        buf.clear();
        probe.processBlock (buf, midi);
        for (int i = 0; i < blockSize; ++i)
        {
            const float l = buf.getSample (0, i), r = buf.getSample (1, i);
            if (std::isfinite (l) && std::isfinite (r)) peak = juce::jmax (peak, std::abs (l), std::abs (r));
            energy.push_back (std::isfinite (l) && std::isfinite (r) ? 0.5 * (l * (double) l + r * (double) r) : 0.0);
        }
    }
    double run = 0.0, best = 0.0;
    for (size_t i = 0; i < energy.size(); ++i)
    {
        run += energy[i];
        if (i >= (size_t) win) run -= energy[i - (size_t) win];
        best = juce::jmax (best, run);
    }
    if (peakDb) *peakDb = juce::Decibels::gainToDecibels (peak, -120.0f);
    return (float) (10.0 * std::log10 (best / win + 1e-20));
}

void XoxicoaSynthProcessor::normaliseLoudness (xs::Patch& patch, int category)
{
    // loudness target (dB RMS of the loudest 400 ms) and a peak target before the limiter
    static constexpr float rmsTarget[]  { -13.0f, -16.0f, -9.5f, -15.0f, -16.0f };
    static constexpr float peakTarget[] { -1.0f, 1.0f, -1.0f, -1.0f, 1.0f };  // limiter rounds off the overs
    const int c = juce::jlimit (0, 4, category);
    float peak = 0.0f;
    const float measured = probeLoudnessDb (patch, &peak);
    const float gain = juce::jmin (rmsTarget[c] - measured, peakTarget[c] - peak);
    patch["master_vol"] = juce::jlimit (-24.0f, 12.0f, gain);
}

juce::File XoxicoaSynthProcessor::userPresetDir()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Xoxicoa Synth").getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

juce::Array<juce::File> XoxicoaSynthProcessor::userPresets() const
{
    auto files = userPresetDir().findChildFiles (juce::File::findFiles, false, "*.xoxpreset");
    files.sort();
    return files;
}

bool XoxicoaSynthProcessor::saveUserPreset (const juce::File& file)
{
    auto f = file.withFileExtension ("xoxpreset");
    juce::XmlElement xml ("XoxicoaPreset");
    xml.setAttribute ("name", f.getFileNameWithoutExtension());
    xml.setAttribute ("version", 1);
    for (auto& d : xs::paramDefs())
        if (d.affectsSound)
        {
            auto* e = xml.createNewChildElement ("p");
            e->setAttribute ("id", d.id);
            e->setAttribute ("v", (double) apvts.getRawParameterValue (d.id)->load());
        }
    if (! xml.writeTo (f)) return false;
    presetName = f.getFileNameWithoutExtension();
    factoryIndex = -1;
    if (onPresetChanged) onPresetChanged();
    return true;
}

bool XoxicoaSynthProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (! xml || ! xml->hasTagName ("XoxicoaPreset")) return false;
    xs::Patch p;
    for (auto* e : xml->getChildWithTagNameIterator ("p"))
        if (xs::findDef (e->getStringAttribute ("id")))
            p[e->getStringAttribute ("id")] = (float) e->getDoubleAttribute ("v");
    factoryIndex = -1;
    applyPatch (p, xml->getStringAttribute ("name", file.getFileNameWithoutExtension()));
    return true;
}

void XoxicoaSynthProcessor::stepPreset (int delta)
{
    const int nf = (int) xs::factoryPresets().size();
    const auto users = userPresets();
    const int total = nf + users.size();
    int cur = factoryIndex;
    if (cur < 0)
    {
        for (int i = 0; i < users.size(); ++i)
            if (users[i].getFileNameWithoutExtension() == presetName) cur = nf + i;
        if (cur < 0) cur = delta > 0 ? -1 : 0;
    }
    const int next = ((cur + delta) % total + total) % total;
    if (next < nf) loadFactory (next);
    else loadUserPreset (users[next - nf]);
}

void XoxicoaSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("factoryIndex", factoryIndex, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void XoxicoaSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            presetName = tree.getProperty ("presetName", "Init").toString();
            factoryIndex = (int) tree.getProperty ("factoryIndex", -1);
            apvts.replaceState (tree);
            if (onPresetChanged) onPresetChanged();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new XoxicoaSynthProcessor(); }
