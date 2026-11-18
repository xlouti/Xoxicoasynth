#pragma once
#include "XLookAndFeel.h"
#include "../dsp/Engine.h"

namespace xs::ui
{
inline void drawGrid (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colour (0xff1a1a1a));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * i / 4.0f), r.getX() + 1, r.getRight() - 1);
    for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * i / 8.0f), r.getY() + 1, r.getBottom() - 1);
    g.setColour (col::line);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

// Live oscilloscope of the synth output, triggered on rising zero crossings.
class Scope : public juce::Component, private juce::Timer
{
public:
    explicit Scope (ScopeBuffer& b) : buf (b) { startTimerHz (30); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawGrid (g, r);
        constexpr int N = 1024, show = 640;
        float s[N];
        const int w = buf.writePos.load (std::memory_order_acquire);
        for (int i = 0; i < N; ++i) s[i] = buf.data[(w - N + i + ScopeBuffer::kSize) & (ScopeBuffer::kSize - 1)];
        int start = 0;
        for (int i = 1; i < N - show; ++i) if (s[i - 1] <= 0.0f && s[i] > 0.0f) { start = i; break; }
        float pk = 0.05f;
        for (int i = 0; i < show; ++i) pk = juce::jmax (pk, std::abs (s[start + i]));
        juce::Path p;
        const float mid = r.getCentreY(), hh = r.getHeight() * 0.42f;
        for (int i = 0; i < show; ++i)
        {
            const float x = r.getX() + r.getWidth() * (float) i / (float) (show - 1);
            const float y = mid - juce::jlimit (-1.0f, 1.0f, s[start + i] / juce::jmax (pk, 0.25f)) * hh;
            i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
        }
        g.setColour (col::fg);
        g.strokePath (p, juce::PathStrokeType (1.5f));
        g.setColour (col::dim);
        g.setFont (font (10.0f, true));
        g.drawText ("OUTPUT", r.reduced (8, 5), juce::Justification::topLeft);
    }
private:
    ScopeBuffer& buf;
    void timerCallback() override { repaint(); }
};

// Shows one cycle of an oscillator's current wavetable position.
class WaveView : public juce::Component, private juce::Timer
{
public:
    WaveView (std::atomic<float>* wave, std::atomic<float>* on) : pos (wave), enabled (on) { startTimerHz (20); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawGrid (g, r);
        const auto& bank = WavetableBank::get();
        juce::Path p;
        const float mid = r.getCentreY(), hh = r.getHeight() * 0.38f;
        const float wp = pos->load();
        for (int i = 0; i <= 128; ++i)
        {
            const float ph = (float) i / 128.0f;
            const float v = bank.read (wp, 6, juce::jmin (ph, 0.9999f));
            const float x = r.getX() + 4 + (r.getWidth() - 8) * ph, y = mid - v * hh;
            i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
        }
        g.setColour (enabled->load() > 0.5f ? col::fg : col::dim);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }
private:
    std::atomic<float>* pos; std::atomic<float>* enabled;
    float last = -1.0f;
    void timerCallback() override { const float v = pos->load() + 10.0f * enabled->load(); if (v != last) { last = v; repaint(); } }
};

// ADSR shape preview.
class EnvView : public juce::Component, private juce::Timer
{
public:
    EnvView (std::atomic<float>* a, std::atomic<float>* d, std::atomic<float>* s, std::atomic<float>* r) : A (a), D (d), S (s), R (r) { startTimerHz (20); }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        drawGrid (g, b);
        b = b.reduced (5, 5);
        auto seg = [] (float ms) { return std::sqrt (juce::jmax (0.0f, ms) / 8000.0f); };
        const float a = seg (A->load()), d = seg (D->load()), s = S->load(), r = seg (R->load()), hold = 0.35f;
        const float total = a + d + hold + r + 1e-3f;
        auto X = [&] (float t) { return b.getX() + b.getWidth() * t / total; };
        auto Y = [&] (float v) { return b.getBottom() - b.getHeight() * v; };
        juce::Path p;
        p.startNewSubPath (X (0), Y (0));
        p.lineTo (X (a), Y (1));
        for (int i = 1; i <= 20; ++i) { const float t = i / 20.0f; p.lineTo (X (a + d * t), Y (s + (1 - s) * std::pow (0.02f, t))); }
        p.lineTo (X (a + d + hold), Y (s));
        for (int i = 1; i <= 20; ++i) { const float t = i / 20.0f; p.lineTo (X (a + d + hold + r * t), Y (s * std::pow (0.01f, t))); }
        g.setColour (col::fg);
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }
private:
    std::atomic<float> *A, *D, *S, *R;
    float last = -1;
    void timerCallback() override { const float v = A->load() + D->load() * 3 + S->load() * 7 + R->load() * 11; if (v != last) { last = v; repaint(); } }
};

class Meter : public juce::Component, private juce::Timer
{
public:
    explicit Meter (float& p) : peak (p) { startTimerHz (30); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff111111));
        g.fillRoundedRectangle (r, 2.0f);
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float h = r.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        g.setColour (col::fg);
        g.fillRoundedRectangle (r.withTop (r.getBottom() - h), 2.0f);
    }
private:
    float& peak;
    float level = 0.0f;
    void timerCallback() override { level = juce::jmax (peak, level * 0.85f); repaint(); }
};
} // namespace xs::ui
