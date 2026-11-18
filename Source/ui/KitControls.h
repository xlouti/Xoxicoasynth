#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "XLookAndFeel.h"

namespace xs::ui
{
// 16-step pattern editor bound to the integer "seq_pat" parameter (bit i = step i).
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid (juce::RangedAudioParameter& p)
        : param (p), attach (p, [this] (float) { repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
        setTooltip ("Loop pattern: click steps on/off (16 steps = one loop)");
    }
    int bits() const { return juce::roundToInt (param.convertFrom0to1 (param.getValue())); }
    void paint (juce::Graphics& g) override
    {
        const int b = bits();
        const float w = (float) getWidth() / 16.0f;
        for (int i = 0; i < 16; ++i)
        {
            auto r = juce::Rectangle<float> ((float) i * w, 0.0f, w, (float) getHeight()).reduced (1.5f, 0.5f);
            const bool on = (b >> i) & 1;
            if (on) { g.setColour (col::fg); g.fillRoundedRectangle (r, 2.0f); }
            g.setColour (on ? col::fg : (i % 4 == 0 ? col::dim : col::line.brighter (0.4f)));
            g.drawRoundedRectangle (r, 2.0f, 1.0f);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, 15, (int) ((float) e.x / ((float) getWidth() / 16.0f)));
        attach.setValueAsCompleteGesture ((float) (bits() ^ (1 << i)));
    }
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attach;
};

// Waveform of the current sound. Click = play, drag = drop the WAV into FL / Finder.
class WaveTile : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onPlay;
    std::function<juce::File()> onDragFile;
    WaveTile() { setTooltip ("Click to play  |  Drag the sound straight into FL Studio's playlist, channel rack or a folder"); setMouseCursor (juce::MouseCursor::DraggingHandCursor); }

    void setBuffer (const juce::AudioBuffer<float>& b)
    {
        const int n = b.getNumSamples();
        peaks.assign (kCols, 0.0f);
        length = n / 44100.0;
        if (n > 0)
            for (int c = 0; c < kCols; ++c)
            {
                const int s0 = (int) ((juce::int64) n * c / kCols), s1 = juce::jmax (s0 + 1, (int) ((juce::int64) n * (c + 1) / kCols));
                peaks[(size_t) c] = juce::jmax (b.getMagnitude (0, s0, s1 - s0), b.getNumChannels() > 1 ? b.getMagnitude (1, s0, s1 - s0) : 0.0f);
            }
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (isMouseOver() ? juce::Colour (0xff121212) : col::bg);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (isMouseOver() ? col::mid : col::line.brighter (0.4f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        auto wr = r.reduced (4.0f, 5.0f).withTrimmedBottom (10.0f);
        g.setColour (col::fg);
        const float cw = wr.getWidth() / (float) kCols;
        for (int c = 0; c < (int) peaks.size(); ++c)
        {
            const float h = juce::jmax (0.5f, peaks[(size_t) c] * wr.getHeight() * 0.5f);
            g.fillRect (wr.getX() + (float) c * cw, wr.getCentreY() - h, juce::jmax (1.0f, cw - 0.4f), h * 2.0f);
        }
        g.setColour (col::dim);
        g.setFont (font (9.5f, true));
        g.drawText ("DRAG  /  CLICK   " + juce::String (length, 2) + " s", r.reduced (5.0f, 2.0f), juce::Justification::bottomLeft);
    }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseDown (const juce::MouseEvent&) override { dragged = false; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragged || e.getDistanceFromDragStart() < 6 || ! onDragFile) return;
        dragged = true;
        const auto f = onDragFile();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const juce::MouseEvent&) override { if (! dragged && onPlay) onPlay(); }
private:
    static constexpr int kCols = 120;
    std::vector<float> peaks;
    double length = 0.0;
    bool dragged = false;
};
} // namespace xs::ui
