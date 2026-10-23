#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace toytune
{

class RotarySliderWithLabel : public juce::Component
{
public:
    RotarySliderWithLabel(const juce::String& labelText, const juce::String& suffix = "")
        : suffixText(suffix)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setDoubleClickReturnValue(true, 0.0);
        addAndMakeVisible(slider);

        nameLabel.setText(labelText, juce::dontSendNotification);
        nameLabel.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9a9aa5));
        addAndMakeVisible(nameLabel);

        valueLabel.setText("0", juce::dontSendNotification);
        valueLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe2e2e8));
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

    void resized() override
    {
        auto bounds = getLocalBounds();
        nameLabel.setBounds(bounds.removeFromTop(16));
        valueLabel.setBounds(bounds.removeFromBottom(16));
        slider.setBounds(bounds);
    }

    juce::Slider& getSlider() noexcept { return slider; }

private:
    juce::Slider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
    juce::String suffixText;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void updateValueDisplay()
    {
        const double val = slider.getValue();
        juce::String text;
        if (std::abs(val) < 10.0 && slider.getInterval() < 0.5)
            text = juce::String(val, 1);
        else
            text = juce::String(static_cast<int>(std::round(val)));

        if (suffixText.isNotEmpty())
            text += " " + suffixText;

        valueLabel.setText(text, juce::dontSendNotification);
    }
};

} // namespace toytune
