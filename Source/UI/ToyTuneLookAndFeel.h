#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace toytune
{

class ToyTuneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ToyTuneLookAndFeel();
    ~ToyTuneLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                           bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    // Brand color palette
    static const juce::Colour backgroundDark;
    static const juce::Colour panelBackground;
    static const juce::Colour panelBorder;
    static const juce::Colour knobDark;
    static const juce::Colour knobBezel;
    static const juce::Colour knobPointer;
    static const juce::Colour accentWhite;
    static const juce::Colour accentSilver;
    static const juce::Colour textMuted;
    static const juce::Colour activeIndicator;
};

} // namespace toytune
