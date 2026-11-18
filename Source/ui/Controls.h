#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "XLookAndFeel.h"
#include "../Params.h"

namespace xs::ui
{
// Rotary knob bound straight to a host parameter.
//  - drag up/right to raise; speed-sensitive (fast drags travel further)
//  - Shift = fine, double-click / Alt-click = reset, mouse wheel supported
//  - shows a value pill while hovered or dragged
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (juce::RangedAudioParameter& p, const juce::String& label, bool big = false)
        : param (p), text (label), large (big),
          attach (p, [this] (float) { repaint(); }, nullptr)
    {
        def = findDef (p.paramID);
        bipolar = def && def->min < 0.0f && def->max > 0.0f;
        attach.sendInitialUpdate();
        setTooltip (p.getName (64));
        setRepaintsOnMouseActivity (true);
    }

    float norm() const { return param.getValue(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float labelH = 16.0f;
        auto knobArea = b.withTrimmedBottom (labelH);
        const float d = juce::jmin (knobArea.getWidth(), knobArea.getHeight()) - 6.0f;
        auto c = knobArea.getCentre();
        const float rad = d * 0.5f;
        const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;
        const float v = norm();
        const float av = a0 + v * (a1 - a0);
        const bool active = dragging || isMouseOverOrDragging();

        juce::Path track;
        track.addCentredArc (c.x, c.y, rad, rad, 0.0f, a0, a1, true);
        g.setColour (col::track);
        g.strokePath (track, juce::PathStrokeType (large ? 3.5f : 2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float from = bipolar ? a0 + (a1 - a0) * param.convertTo0to1 (0.0f) : a0;
        juce::Path val;
        val.addCentredArc (c.x, c.y, rad, rad, 0.0f, juce::jmin (from, av), juce::jmax (from, av), true);
        g.setColour (col::fg);
        g.strokePath (val, juce::PathStrokeType (large ? 3.5f : 2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // body
        const float br = rad * 0.72f;
        g.setColour (active ? juce::Colour (0xff1c1c1c) : juce::Colour (0xff111111));
        g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
        g.setColour (active ? col::mid : col::line.brighter (0.3f));
        g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.0f);
        // pointer
        const juce::Point<float> p1 (c.x + std::sin (av) * br * 0.25f, c.y - std::cos (av) * br * 0.25f);
        const juce::Point<float> p2 (c.x + std::sin (av) * br * 0.92f, c.y - std::cos (av) * br * 0.92f);
        g.setColour (col::fg);
        g.drawLine ({ p1, p2 }, large ? 2.5f : 2.0f);

        auto lr = b.removeFromBottom (labelH);
        if (active)
        {
            const auto s = param.getCurrentValueAsText();
            const float tw = juce::jmin (b.getWidth() + 30.0f, (float) font (11.0f, true).getStringWidth (s) + 12.0f);
            auto pill = juce::Rectangle<float> (tw, labelH - 2.0f).withCentre (lr.getCentre());
            g.setColour (col::fg);
            g.fillRoundedRectangle (pill, 3.0f);
            g.setColour (col::bg);
            g.setFont (font (11.0f, true));
            g.drawText (s, pill, juce::Justification::centred, false);
        }
        else
        {
            g.setColour (col::mid);
            g.setFont (font (11.0f));
            g.drawText (text.toUpperCase(), lr, juce::Justification::centred, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isAltDown() || e.mods.isCommandDown()) { resetToDefault(); return; }
        dragging = true;
        accum = norm();
        lastPos = e.position;
        lastTime = juce::Time::getMillisecondCounterHiRes();
        attach.beginGesture();
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return;
        const auto now = juce::Time::getMillisecondCounterHiRes();
        const auto delta = e.position - lastPos;
        const float px = delta.x - delta.y;                                  // up or right = more
        const float dt = (float) juce::jmax (1.0, now - lastTime);
        const float speed = std::abs (px) / dt;                              // px per ms
        const bool fine = e.mods.isShiftDown();
        float sens = fine ? 1.0f / 900.0f : 1.0f / 220.0f;
        if (! fine) sens *= juce::jlimit (0.6f, 2.6f, 0.7f + speed * 0.9f); // velocity response
        accum = juce::jlimit (0.0f, 1.0f, accum + px * sens);
        setNorm (accum, true);
        lastPos = e.position;
        lastTime = now;
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging) attach.endGesture();
        dragging = false;
        repaint();
    }

    void mouseDoubleClick (const juce::MouseEvent&) override { resetToDefault(); }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const float dir = (w.deltaY != 0.0f ? w.deltaY : -w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
        const float step = e.mods.isShiftDown() ? 0.004f : 0.025f;
        const float nv = juce::jlimit (0.0f, 1.0f, norm() + (dir > 0 ? step : -step) * juce::jmax (1.0f, std::abs (dir) * 4.0f));
        attach.setValueAsCompleteGesture (param.convertFrom0to1 (nv));
    }

private:
    juce::RangedAudioParameter& param;
    juce::String text;
    bool large = false, bipolar = false, dragging = false;
    const ParamDef* def = nullptr;
    juce::ParameterAttachment attach;
    float accum = 0.0f;
    juce::Point<float> lastPos;
    double lastTime = 0.0;

    void setNorm (float n, bool inGesture)
    {
        const float v = param.convertFrom0to1 (n);
        if (inGesture) attach.setValueAsPartOfGesture (v);
        else attach.setValueAsCompleteGesture (v);
    }
    void resetToDefault() { attach.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
};

// Stepper for choice / integer params:  < value >  ; click the middle for a menu.
class Selector : public juce::Component, public juce::SettableTooltipClient
{
public:
    Selector (juce::RangedAudioParameter& p, const juce::String& label = {})
        : param (p), text (label), attach (p, [this] (float) { repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
        setTooltip (p.getName (64));
        setRepaintsOnMouseActivity (true);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        if (text.isNotEmpty())
        {
            g.setColour (col::dim);
            g.setFont (font (10.5f));
            g.drawText (text.toUpperCase(), b.removeFromLeft (labelW), juce::Justification::centredLeft);
        }
        b = b.reduced (0.5f);
        g.setColour (isMouseOver() ? juce::Colour (0xff121212) : col::bg);
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (isMouseOver() ? col::mid : col::line.brighter (0.4f));
        g.drawRoundedRectangle (b, 3.0f, 1.0f);
        g.setColour (col::fg);
        g.setFont (font (12.0f, true));
        g.drawText (param.getCurrentValueAsText(), b.reduced (14, 0), juce::Justification::centred, true);
        g.setColour (col::dim);
        auto arrow = [&g] (juce::Rectangle<float> r, bool left)
        {
            juce::Path t;
            const auto cc = r.getCentre();
            if (left) t.addTriangle (cc.x + 2, cc.y - 3.5f, cc.x + 2, cc.y + 3.5f, cc.x - 2.5f, cc.y);
            else      t.addTriangle (cc.x - 2, cc.y - 3.5f, cc.x - 2, cc.y + 3.5f, cc.x + 2.5f, cc.y);
            g.fillPath (t);
        };
        arrow (b.withWidth (14), true);
        arrow (b.withTrimmedLeft (b.getWidth() - 14), false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto b = getLocalBounds().withTrimmedLeft (text.isNotEmpty() ? labelW : 0);
        const int x = e.x - b.getX();
        if (x < 16) step (-1);
        else if (x > b.getWidth() - 16) step (1);
        else showMenu();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        step ((w.deltaY != 0 ? w.deltaY : -w.deltaX) > 0 ? 1 : -1);
    }

    int labelW = 0;

private:
    juce::RangedAudioParameter& param;
    juce::String text;
    juce::ParameterAttachment attach;

    int numSteps() const { return juce::jmax (2, param.getNumSteps()); }
    int current() const { return juce::roundToInt (param.getValue() * (float) (numSteps() - 1)); }
    void setIndex (int i)
    {
        i = juce::jlimit (0, numSteps() - 1, i);
        attach.setValueAsCompleteGesture (param.convertFrom0to1 ((float) i / (float) (numSteps() - 1)));
    }
    void step (int d) { setIndex (current() + d); }
    void showMenu()
    {
        juce::PopupMenu m;
        const int n = numSteps();
        for (int i = 0; i < n; ++i)
            m.addItem (i + 1, param.getText ((float) i / (float) (n - 1), 32), true, i == current());
        juce::Component::SafePointer<Selector> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                         [safe] (int r) { if (safe && r > 0) safe->setIndex (r - 1); });
    }
};

// Small on/off switch bound to a bool param.
class Toggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    Toggle (juce::RangedAudioParameter& p, const juce::String& label = {})
        : param (p), text (label), attach (p, [this] (float) { repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
        setTooltip (p.getName (64));
        setRepaintsOnMouseActivity (true);
    }
    bool isOn() const { return param.getValue() > 0.5f; }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto box = b.removeFromLeft (b.getHeight()).reduced (2.0f);
        g.setColour (isOn() ? col::fg : (isMouseOver() ? col::mid : col::dim));
        g.drawRoundedRectangle (box, 2.5f, 1.2f);
        if (isOn()) g.fillRoundedRectangle (box.reduced (2.5f), 1.5f);
        if (text.isNotEmpty())
        {
            g.setColour (isOn() ? col::fg : col::dim);
            g.setFont (font (10.5f, true));
            g.drawText (text.toUpperCase(), b.withTrimmedLeft (5), juce::Justification::centredLeft);
        }
    }
    void mouseDown (const juce::MouseEvent&) override { attach.setValueAsCompleteGesture (isOn() ? 0.0f : 1.0f); }

private:
    juce::RangedAudioParameter& param;
    juce::String text;
    juce::ParameterAttachment attach;
};
} // namespace xs::ui
