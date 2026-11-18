#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Presets.h"
#include "dsp/Engine.h"

class XoxicoaSynthProcessor : public juce::AudioProcessor
{
public:
    XoxicoaSynthProcessor();
    ~XoxicoaSynthProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Xoxicoa Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return (int) xs::factoryPresets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, factoryIndex); }
    void setCurrentProgram (int index) override { loadFactory (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- presets / generator (message thread)
    void applyPatch (const xs::Patch& patch, const juce::String& name);
    void loadFactory (int index);
    void generate();
    bool saveUserPreset (const juce::File& file);
    bool loadUserPreset (const juce::File& file);
    static juce::File userPresetDir();
    juce::Array<juce::File> userPresets() const;
    void stepPreset (int delta);

    // Loudness of a patch (loudest 400 ms window, dB RMS) measured by rendering it offline.
    static float probeLoudnessDb (const xs::Patch& patch, float* peakDb = nullptr);
    // Sets master_vol so the patch lands on its category's loudness target.
    static void normaliseLoudness (xs::Patch& patch, int category);

    juce::String presetName { "Init" };
    int factoryIndex = -1;
    std::function<void()> onPresetChanged;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    xs::Engine engine;

private:
    juce::Random genRng;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XoxicoaSynthProcessor)
};
