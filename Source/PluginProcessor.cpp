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
    normaliseLoudness (patch, cat);
    factoryIndex = -1;
    applyPatch (patch, "Gen: " + xs::categories[cat] + " #" + juce::String (genRng.nextInt ({ 100, 999 })));
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
