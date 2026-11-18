#pragma once
#include "PluginProcessor.h"
#include "ui/XLookAndFeel.h"
#include "ui/Controls.h"
#include "ui/Displays.h"

class XoxicoaSynthEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit XoxicoaSynthEditor (XoxicoaSynthProcessor&);
    ~XoxicoaSynthEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kW = 1240, kH = 720;

private:
    XoxicoaSynthProcessor& proc;
    xs::ui::XLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    struct Panel { juce::String title; juce::Rectangle<int> bounds; juce::String toggleId; };
    std::vector<Panel> panels;

    juce::OwnedArray<juce::Component> owned;
    std::map<juce::String, juce::Component*> byId;

    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, presetBtn, saveBtn { "SAVE" }, loadBtn { "LOAD" }, generateBtn { "GENERATE" };
    std::unique_ptr<xs::ui::Scope> scope;
    std::unique_ptr<xs::ui::WaveView> waveA, waveB;
    std::unique_ptr<xs::ui::EnvView> env1View, env2View;
    std::unique_ptr<xs::ui::Meter> meter;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String shownName;

    juce::Component& knob (const char* id, const char* label, bool big = false);
    juce::Component& sel (const char* id, const char* label = "");
    juce::Component& tog (const char* id, const char* label = "");
    void place (const char* id, juce::Rectangle<int> r) { if (auto* c = byId[id]) c->setBounds (r); }
    void showPresetMenu();
    void updatePresetLabel();
    void timerCallback() override;
};
