// Renders the real Xoxicoa Synth editor to a PNG without opening a window.
// usage: XoxicoaSynthUISnapshot out.png [scale] [presetIndex]
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? juce::String (argv[1]) : juce::String ("XoxicoaSynth_ui.png"));
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 2.0f;
    XoxicoaSynthProcessor proc;
    proc.loadFactory (argc > 3 ? std::atoi (argv[3]) : 0);
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
    m.addEvent (juce::MidiMessage::noteOn (1, 63, (juce::uint8) 100), 0);
    m.addEvent (juce::MidiMessage::noteOn (1, 67, (juce::uint8) 100), 0);
    for (int blk = 0; blk < 60; ++blk) { proc.processBlock (b, m); m.clear(); }
    proc.keyboardState.noteOn (1, 60, 1.0f); proc.keyboardState.noteOn (1, 63, 1.0f); proc.keyboardState.noteOn (1, 67, 1.0f);

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setVisible (true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (500);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat png;
    const bool ok = os.openedOk() && png.writeImageToStream (img, os);
    std::printf ("%s %s (%dx%d)\n", ok ? "wrote" : "FAILED", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    ed.reset();
    return ok ? 0 : 1;
}
