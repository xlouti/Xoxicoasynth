#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Strict monochrome look: black canvas, white controls/text, grey for secondary.
namespace xs::ui
{
namespace col
{
    inline const juce::Colour bg       { 0xff000000 };
    inline const juce::Colour panel    { 0xff0a0a0a };
    inline const juce::Colour line     { 0xff2a2a2a };
    inline const juce::Colour track    { 0xff333333 };
    inline const juce::Colour dim      { 0xff7a7a7a };
    inline const juce::Colour mid      { 0xffb0b0b0 };
    inline const juce::Colour fg       { 0xffffffff };
}

inline juce::Font font (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

class XLookAndFeel : public juce::LookAndFeel_V4
{
public:
    XLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, col::bg);
        setColour (juce::PopupMenu::backgroundColourId, col::bg);
        setColour (juce::PopupMenu::textColourId, col::fg);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::fg);
        setColour (juce::PopupMenu::highlightedTextColourId, col::bg);
        setColour (juce::PopupMenu::headerTextColourId, col::dim);
        setColour (juce::TextButton::buttonColourId, col::bg);
        setColour (juce::TextButton::textColourOffId, col::fg);
        setColour (juce::TextButton::textColourOnId, col::bg);
        setColour (juce::TooltipWindow::backgroundColourId, col::fg);
        setColour (juce::TooltipWindow::textColourId, col::bg);
        setColour (juce::AlertWindow::backgroundColourId, col::bg);
        setColour (juce::AlertWindow::textColourId, col::fg);
        setColour (juce::AlertWindow::outlineColourId, col::fg);
        setColour (juce::TextEditor::backgroundColourId, col::bg);
        setColour (juce::TextEditor::textColourId, col::fg);
        setColour (juce::TextEditor::outlineColourId, col::dim);
        setColour (juce::TextEditor::focusedOutlineColourId, col::fg);
        setColour (juce::TextEditor::highlightColourId, col::mid.withAlpha (0.4f));
        setColour (juce::CaretComponent::caretColourId, col::fg);
        setColour (juce::ListBox::backgroundColourId, col::bg);
        setColour (juce::ListBox::textColourId, col::fg);
        setColour (juce::ScrollBar::thumbColourId, col::dim);
        setColour (juce::FileTreeComponent::highlightColourId, col::fg);
        setColour (juce::DirectoryContentsDisplayComponent::highlightColourId, col::fg);
        setColour (juce::DirectoryContentsDisplayComponent::textColourId, col::fg);
        setColour (juce::DirectoryContentsDisplayComponent::highlightedTextColourId, col::bg);
        setColour (juce::ComboBox::backgroundColourId, col::bg);
        setColour (juce::ComboBox::textColourId, col::fg);
        setColour (juce::ComboBox::outlineColourId, col::dim);
        setColour (juce::ComboBox::arrowColourId, col::fg);
        setColour (juce::Label::textColourId, col::fg);
        setColour (juce::ResizableWindow::backgroundColourId, col::bg);
        setColour (juce::DocumentWindow::textColourId, col::fg);
        getCurrentColourScheme().setUIColour (ColourScheme::windowBackground, col::bg);
        getCurrentColourScheme().setUIColour (ColourScheme::widgetBackground, col::bg);
        getCurrentColourScheme().setUIColour (ColourScheme::menuBackground, col::bg);
        getCurrentColourScheme().setUIColour (ColourScheme::outline, col::dim);
        getCurrentColourScheme().setUIColour (ColourScheme::defaultText, col::fg);
        getCurrentColourScheme().setUIColour (ColourScheme::defaultFill, col::fg);
        getCurrentColourScheme().setUIColour (ColourScheme::highlightedText, col::bg);
        getCurrentColourScheme().setUIColour (ColourScheme::highlightedFill, col::fg);
        getCurrentColourScheme().setUIColour (ColourScheme::menuText, col::fg);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool primary = b.getProperties()["primary"];
        if (primary || b.getToggleState())
        {
            g.setColour (down ? col::mid : (over ? juce::Colour (0xffe6e6e6) : col::fg));
            g.fillRoundedRectangle (r, 3.0f);
        }
        else
        {
            g.setColour (down ? col::line : (over ? juce::Colour (0xff141414) : col::bg));
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (over ? col::fg : col::dim);
            g.drawRoundedRectangle (r, 3.0f, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        const bool primary = b.getProperties()["primary"];
        g.setColour (primary || b.getToggleState() ? col::bg : col::fg);
        g.setFont (font (primary ? 14.0f : 11.5f, true));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
    }

    juce::Font getPopupMenuFont() override { return font (14.0f); }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (col::bg);
        g.setColour (col::dim);
        g.drawRect (0, 0, w, h, 1);
    }

    void drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name) override
    {
        g.setColour (col::dim);
        g.setFont (font (11.0f, true));
        g.drawText (name.toUpperCase(), area.reduced (10, 0), juce::Justification::bottomLeft);
    }

    int getPopupMenuBorderSize() override { return 4; }

    void drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h) override
    {
        g.fillAll (col::fg);
        g.setColour (col::bg);
        g.setFont (font (12.0f));
        g.drawText (text, 0, 0, w, h, juce::Justification::centred);
    }

    void drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                        int thumbStart, int thumbSize, bool over, bool) override
    {
        g.setColour (over ? col::mid : col::dim);
        if (vertical) g.fillRoundedRectangle ((float) x + 2, (float) thumbStart, (float) w - 4, (float) thumbSize, 2.0f);
        else          g.fillRoundedRectangle ((float) thumbStart, (float) y + 2, (float) thumbSize, (float) h - 4, 2.0f);
    }
};
} // namespace xs::ui
