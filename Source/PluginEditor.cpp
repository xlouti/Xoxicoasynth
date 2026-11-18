#include "PluginEditor.h"

using namespace xs::ui;

XoxicoaSynthEditor::XoxicoaSynthEditor (XoxicoaSynthProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);

    auto raw = [&p] (const char* id) { return p.apvts.getRawParameterValue (id); };

    // ---- oscillators
    for (auto pre : { "a", "b" })
    {
        const juce::String s (pre);
        knob ((s + "_wave").toRawUTF8(), "Wave");
        knob ((s + "_fine").toRawUTF8(), "Fine");
        knob ((s + "_level").toRawUTF8(), "Level");
        knob ((s + "_detune").toRawUTF8(), "Detune");
        knob ((s + "_width").toRawUTF8(), "Width");
        sel ((s + "_oct").toRawUTF8(), "Octave");
        sel ((s + "_semi").toRawUTF8(), "Semi");
        sel ((s + "_uni").toRawUTF8(), "Unison");
        tog ((s + "_on").toRawUTF8());
    }
    waveA = std::make_unique<WaveView> (raw ("a_wave"), raw ("a_on"));
    waveB = std::make_unique<WaveView> (raw ("b_wave"), raw ("b_on"));
    addAndMakeVisible (*waveA); addAndMakeVisible (*waveB);

    knob ("sub_level", "Sub"); knob ("sub_shape", "Shape"); sel ("sub_oct");
    knob ("noise_level", "Noise"); knob ("noise_color", "Colour");

    sel ("flt_type");
    knob ("flt_cutoff", "Cutoff", true); knob ("flt_res", "Reso"); knob ("flt_drive", "Drive");
    knob ("flt_env", "Env Amt"); knob ("flt_key", "Key Trk");

    knob ("e1_a", "Attack"); knob ("e1_d", "Decay"); knob ("e1_s", "Sustain"); knob ("e1_r", "Release");
    knob ("e2_a", "Attack"); knob ("e2_d", "Decay"); knob ("e2_s", "Sustain"); knob ("e2_r", "Release");
    env1View = std::make_unique<EnvView> (raw ("e1_a"), raw ("e1_d"), raw ("e1_s"), raw ("e1_r"));
    env2View = std::make_unique<EnvView> (raw ("e2_a"), raw ("e2_d"), raw ("e2_s"), raw ("e2_r"));
    addAndMakeVisible (*env1View); addAndMakeVisible (*env2View);

    knob ("penv_amt", "Amount"); knob ("penv_decay", "Decay");

    sel ("lfo_shape"); sel ("lfo_dest"); knob ("lfo_rate", "Rate"); knob ("lfo_amt", "Amount");
    tog ("lfo_sync", "Sync"); sel ("lfo_div");

    sel ("voice_mode"); sel ("bend", "Bend"); knob ("glide", "Glide"); knob ("vel_sens", "Velocity");

    tog ("dist_on"); sel ("dist_type"); knob ("dist_drive", "Drive"); knob ("dist_tone", "Tone"); knob ("dist_mix", "Mix");
    tog ("cho_on"); knob ("cho_rate", "Rate"); knob ("cho_depth", "Depth"); knob ("cho_mix", "Mix");
    tog ("dly_on"); sel ("dly_div"); tog ("dly_pp", "Ping Pong"); knob ("dly_fb", "Feedback"); knob ("dly_tone", "Tone"); knob ("dly_mix", "Mix");
    tog ("rev_on"); knob ("rev_size", "Size"); knob ("rev_damp", "Damp"); knob ("rev_width", "Width"); knob ("rev_mix", "Mix");

    knob ("master_vol", "Master"); tog ("limiter", "Limit");
    sel ("gen_cat"); knob ("gen_wild", "Wild");

    scope = std::make_unique<Scope> (p.engine.scope);
    addAndMakeVisible (*scope);
    meter = std::make_unique<Meter> (p.engine.outPeak);
    addAndMakeVisible (*meter);

    for (auto* b : { &prevBtn, &nextBtn, &presetBtn, &saveBtn, &loadBtn, &generateBtn })
        addAndMakeVisible (*b);
    generateBtn.getProperties().set ("primary", true);
    generateBtn.setTooltip ("Roll a new sound in the selected category. Wild = how far it strays.");
    prevBtn.onClick = [this] { proc.stepPreset (-1); };
    nextBtn.onClick = [this] { proc.stepPreset (1); };
    presetBtn.onClick = [this] { showPresetMenu(); };
    generateBtn.onClick = [this] { proc.generate(); };
    saveBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Save preset", XoxicoaSynthProcessor::userPresetDir()
                                                         .getChildFile (proc.presetName.replaceCharacters (":#/", "  -").trim() + ".xoxpreset"), "*.xoxpreset");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult() != juce::File()) proc.saveUserPreset (fc.getResult()); });
    };
    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load preset", XoxicoaSynthProcessor::userPresetDir(), "*.xoxpreset");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) proc.loadUserPreset (fc.getResult()); });
    };

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colours::white);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff0c0c0c));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff3a3a3a));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x40808080));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour (0xff8a8a8a));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, juce::Colour (0xff6a6a6a));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colour (0x00000000));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, juce::Colours::black);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, juce::Colours::white);
    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (24);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setWantsKeyboardFocus (true);
    addAndMakeVisible (keyboard);

    panels = {
        { "OSC A", { 12, 68, 318, 196 }, "a_on" },     { "OSC B", { 338, 68, 318, 196 }, "b_on" },
        { "SUB / NOISE", { 664, 68, 200, 196 }, "" },  { "SCOPE", { 872, 68, 356, 196 }, "" },
        { "FILTER", { 12, 272, 330, 160 }, "" },       { "AMP ENV", { 350, 272, 250, 160 }, "" },
        { "MOD ENV  >  FILTER", { 608, 272, 250, 160 }, "" }, { "PITCH ENV", { 866, 272, 130, 160 }, "" },
        { "LFO", { 1004, 272, 224, 160 }, "" },
        { "VOICE", { 12, 440, 230, 132 }, "" },        { "DISTORTION", { 250, 440, 240, 132 }, "dist_on" },
        { "CHORUS", { 498, 440, 190, 132 }, "cho_on" }, { "DELAY", { 696, 440, 270, 132 }, "dly_on" },
        { "REVERB", { 974, 440, 254, 132 }, "rev_on" },
    };

    proc.onPresetChanged = [this] { juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<XoxicoaSynthEditor> (this)] { if (safe) safe->updatePresetLabel(); }); };
    updatePresetLabel();
    setSize (kW, kH);
    startTimerHz (10);
}

XoxicoaSynthEditor::~XoxicoaSynthEditor()
{
    proc.onPresetChanged = nullptr;
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

juce::Component& XoxicoaSynthEditor::knob (const char* id, const char* label, bool big)
{
    auto* k = owned.add (new Knob (*proc.apvts.getParameter (id), label, big));
    byId[id] = k; addAndMakeVisible (k); return *k;
}
juce::Component& XoxicoaSynthEditor::sel (const char* id, const char* label)
{
    auto* s = new Selector (*proc.apvts.getParameter (id), label);
    owned.add (s);
    if (juce::String (label).isNotEmpty()) s->labelW = 52;
    byId[id] = s; addAndMakeVisible (s); return *s;
}
juce::Component& XoxicoaSynthEditor::tog (const char* id, const char* label)
{
    auto* t = owned.add (new Toggle (*proc.apvts.getParameter (id), label));
    byId[id] = t; addAndMakeVisible (t); return *t;
}

void XoxicoaSynthEditor::updatePresetLabel()
{
    shownName = proc.presetName;
    presetBtn.setButtonText (shownName.toUpperCase());
}

void XoxicoaSynthEditor::timerCallback()
{
    if (shownName != proc.presetName) updatePresetLabel();
    repaint (0, 60, getWidth(), 520);  // panel titles follow their on/off switches
}

void XoxicoaSynthEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    const auto& f = xs::factoryPresets();
    for (int c = 0; c < xs::categories.size(); ++c)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < (int) f.size(); ++i)
            if (f[(size_t) i].category == c)
                sub.addItem (i + 1, f[(size_t) i].name, true, proc.factoryIndex == i);
        menu.addSubMenu (xs::categories[c], sub);
    }
    const auto users = proc.userPresets();
    juce::PopupMenu userMenu;
    for (int i = 0; i < users.size(); ++i)
        userMenu.addItem (10000 + i, users[i].getFileNameWithoutExtension());
    if (users.isEmpty()) userMenu.addItem (-1, "(save a preset to see it here)", false);
    menu.addSeparator();
    menu.addSubMenu ("User", userMenu);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn).withMinimumWidth (presetBtn.getWidth()),
        [this, users] (int r)
        {
            if (r >= 10000) proc.loadUserPreset (users[r - 10000]);
            else if (r > 0) proc.loadFactory (r - 1);
        });
}

void XoxicoaSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);

    // header
    g.setColour (col::fg);
    g.setFont (font (24.0f, true));
    g.drawText ("XOXICOA", 18, 14, 130, 32, juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (font (24.0f));
    g.drawText ("SYNTH", 136, 14, 100, 32, juce::Justification::centredLeft);
    g.setFont (font (10.0f, true));
    g.drawText ("GENERATOR", 800, 4, 130, 12, juce::Justification::centredLeft);
    g.drawText ("PRESET", 300, 4, 130, 12, juce::Justification::centredLeft);
    g.setColour (col::line);
    g.drawHorizontalLine (60, 12.0f, (float) getWidth() - 12.0f);

    for (auto& p : panels)
    {
        auto r = p.bounds.toFloat();
        g.setColour (col::panel);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (col::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        bool on = true;
        if (p.toggleId.isNotEmpty()) on = proc.apvts.getRawParameterValue (p.toggleId)->load() > 0.5f;
        g.setColour (on ? col::fg : col::dim);
        g.setFont (font (11.5f, true));
        const int titleX = p.toggleId.isNotEmpty() ? 32 : 10;
        g.drawText (p.title, p.bounds.getX() + titleX, p.bounds.getY() + 6, p.bounds.getWidth() - titleX - 10, 16, juce::Justification::centredLeft);
    }
}

void XoxicoaSynthEditor::resized()
{
    constexpr int KH = 70;
    auto knobRow = [this] (std::initializer_list<const char*> ids, int x, int y, int w = 58, int h = 70)
    {
        for (auto id : ids) { place (id, { x, y, w, h }); x += w; }
    };

    // header
    prevBtn.setBounds (300, 18, 28, 30);
    presetBtn.setBounds (332, 18, 260, 30);
    nextBtn.setBounds (596, 18, 28, 30);
    saveBtn.setBounds (632, 18, 64, 30);
    loadBtn.setBounds (700, 18, 64, 30);
    place ("gen_cat", { 800, 20, 132, 26 });
    place ("gen_wild", { 936, 0, 54, 60 });
    generateBtn.setBounds (996, 16, 112, 32);
    place ("master_vol", { 1114, 0, 54, 60 });
    meter->setBounds (1172, 10, 6, 40);
    place ("limiter", { 1184, 24, 52, 16 });

    // oscillators
    for (int i = 0; i < 2; ++i)
    {
        const int x = i == 0 ? 12 : 338, y = 68;
        const juce::String s (i == 0 ? "a" : "b");
        place ((s + "_on").toRawUTF8(), { x + 10, y + 5, 18, 18 });
        (i == 0 ? waveA : waveB)->setBounds (x + 10, y + 32, 132, 70);
        place ((s + "_oct").toRawUTF8(), { x + 152, y + 32, 156, 20 });
        place ((s + "_semi").toRawUTF8(), { x + 152, y + 57, 156, 20 });
        place ((s + "_uni").toRawUTF8(), { x + 152, y + 82, 156, 20 });
        const int kx = x + 12;
        int cx = kx;
        for (auto id : { "_wave", "_fine", "_level", "_detune", "_width" })
        {
            place ((s + id).toRawUTF8(), { cx, y + 116, 59, KH });
            cx += 59;
        }
    }
    // sub / noise
    place ("sub_level", { 676, 96, 84, KH });
    place ("sub_shape", { 676, 168, 84, KH });
    place ("sub_oct",   { 680, 238, 76, 20 });
    place ("noise_level", { 768, 96, 84, KH });
    place ("noise_color", { 768, 168, 84, KH });
    scope->setBounds (882, 94, 336, 160);

    // filter
    place ("flt_type", { 108, 278, 224, 20 });
    place ("flt_cutoff", { 18, 304, 86, 100 });
    knobRow ({ "flt_res", "flt_drive", "flt_env", "flt_key" }, 106, 322);
    // envelopes
    env1View->setBounds (360, 300, 230, 46);
    knobRow ({ "e1_a", "e1_d", "e1_s", "e1_r" }, 359, 354, 58);
    env2View->setBounds (618, 300, 230, 46);
    knobRow ({ "e2_a", "e2_d", "e2_s", "e2_r" }, 617, 354, 58);
    // pitch env
    knobRow ({ "penv_amt", "penv_decay" }, 873, 330, 58);
    // lfo
    place ("lfo_shape", { 1014, 300, 100, 20 });
    place ("lfo_dest",  { 1118, 300, 100, 20 });
    knobRow ({ "lfo_rate", "lfo_amt" }, 1012, 340, 58);
    place ("lfo_sync", { 1134, 346, 84, 18 });
    place ("lfo_div",  { 1134, 372, 84, 20 });

    // voice
    place ("voice_mode", { 22, 468, 98, 20 });
    place ("bend", { 126, 468, 106, 20 });
    if (auto* s = dynamic_cast<Selector*> (byId["bend"])) s->labelW = 34;
    knobRow ({ "glide", "vel_sens" }, 40, 496, 70);
    // fx
    place ("dist_on", { 260, 445, 18, 18 });
    place ("dist_type", { 260, 468, 220, 20 });
    knobRow ({ "dist_drive", "dist_tone", "dist_mix" }, 268, 496, 68);
    place ("cho_on", { 508, 445, 18, 18 });
    knobRow ({ "cho_rate", "cho_depth", "cho_mix" }, 506, 480, 58);
    place ("dly_on", { 706, 445, 18, 18 });
    place ("dly_div", { 706, 468, 120, 20 });
    place ("dly_pp", { 836, 469, 120, 18 });
    knobRow ({ "dly_fb", "dly_tone", "dly_mix" }, 722, 496, 74);
    place ("rev_on", { 984, 445, 18, 18 });
    knobRow ({ "rev_size", "rev_damp", "rev_width", "rev_mix" }, 984, 480, 59);

    // keyboard
    keyboard.setBounds (12, 582, getWidth() - 24, 126);
    keyboard.setKeyWidth ((float) (getWidth() - 24) / 50.0f);
}

juce::AudioProcessorEditor* XoxicoaSynthProcessor::createEditor() { return new XoxicoaSynthEditor (*this); }
