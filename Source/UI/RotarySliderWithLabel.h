#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace toytune
{

/**
 * Premium Luxury Rotary Knob matching Producer Toy ToyTune reference image.
 * Features:
 * - Label on top (e.g. "Retune Speed", "Humanize", "Flex Tune")
 * - Center Rotary Slider with white arc and radial needle
 * - Value capsule pill below (e.g. "30", "35", "100", "0.0 dB")
 */
class RotarySliderWithLabel : public juce::Component
{
public:
    RotarySliderWithLabel(const juce::String& labelText,
                          const juce::String& suffix = "",
                          bool showDecimals = false,
                          bool isLargeCard = false)
        : suffixText(suffix),
          hasDecimals(showDecimals),
          isLarge(isLargeCard)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setDoubleClickReturnValue(true, 0.0);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                                   juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible(slider);

        nameLabel.setText(labelText, juce::dontSendNotification);
        nameLabel.setFont(juce::Font(juce::FontOptions(isLarge ? 13.0f : 11.5f).withStyle("Bold")));
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd5d8e2));
        addAndMakeVisible(nameLabel);

        valueLabel.setText("0", juce::dontSendNotification);
        valueLabel.setFont(juce::Font(juce::FontOptions(isLarge ? 13.5f : 12.0f).withStyle("Bold")));
        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
        addAndMakeVisible(valueLabel);

        slider.onValueChange = [this]()
        {
            updateValueDisplay();
        };
    }

    void attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
        updateValueDisplay();
    }

    void enablementChanged() override
    {
        const bool enabled = isEnabled();
        slider.setEnabled(enabled);

        nameLabel.setColour(juce::Label::textColourId,
            enabled ? juce::Colour(0xffd5d8e2) : juce::Colour(0xff4c4f5a));

        valueLabel.setColour(juce::Label::textColourId,
            enabled ? juce::Colour(0xffffffff) : juce::Colour(0xff4c4f5a));

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const bool enabled = isEnabled();

        // Draw card background if this is a large card (Retune Speed / Humanize)
        if (isLarge)
        {
            const auto bounds = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0xff101114));
            g.fillRoundedRectangle(bounds, 12.0f);
            g.setColour(juce::Colour(0xff1e2027));
            g.drawRoundedRectangle(bounds, 12.0f, 1.2f);
        }

        // Draw value pill background below knob
        const auto pillBounds = valueLabel.getBounds().toFloat().reduced(2.0f, 1.0f);
        if (enabled)
        {
            g.setColour(juce::Colour(0xff16171b));
            g.fillRoundedRectangle(pillBounds, pillBounds.getHeight() * 0.5f);
            g.setColour(juce::Colour(0xff272932));
            g.drawRoundedRectangle(pillBounds, pillBounds.getHeight() * 0.5f, 1.0f);
        }
        else
        {
            g.setColour(juce::Colour(0xff0e0f12));
            g.fillRoundedRectangle(pillBounds, pillBounds.getHeight() * 0.5f);
            g.setColour(juce::Colour(0xff1a1b20));
            g.drawRoundedRectangle(pillBounds, pillBounds.getHeight() * 0.5f, 1.0f);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        if (isLarge)
            bounds.reduce(8, 10);

        nameLabel.setBounds(bounds.removeFromTop(isLarge ? 22 : 18));
        
        const int pillHeight = isLarge ? 24 : 20;
        const int pillWidth = isLarge ? 58 : 50;
        auto bottomArea = bounds.removeFromBottom(pillHeight + 4);
        valueLabel.setBounds(bottomArea.getCentreX() - (pillWidth / 2),
                             bottomArea.getCentreY() - (pillHeight / 2),
                             pillWidth, pillHeight);

        slider.setBounds(bounds);
    }

    juce::Slider& getSlider() noexcept { return slider; }

private:
    juce::Slider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
    juce::String suffixText;
    bool hasDecimals { false };
    bool isLarge { false };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void updateValueDisplay()
    {
        const double val = slider.getValue();
        juce::String text;
        if (hasDecimals)
            text = juce::String(val, 1);
        else
            text = juce::String(static_cast<int>(std::round(val)));

        if (suffixText.isNotEmpty())
            text += " " + suffixText;

        valueLabel.setText(text, juce::dontSendNotification);
        repaint();
    }
};

} // namespace toytune
