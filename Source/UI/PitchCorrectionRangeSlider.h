#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace toytune
{

/**
 * Pitch Correction Range Slider matching Producer Toy ToyTune reference image.
 * Features:
 * - Label: "Pitch Correction Range"
 * - Horizontal track with white pill slider thumb
 * - Scale markings below: -100, -50, 0, +50, +100
 */
class PitchCorrectionRangeSlider : public juce::Component
{
public:
    PitchCorrectionRangeSlider()
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);

        titleLabel.setText("Pitch Correction Range", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(juce::FontOptions(11.5f).withStyle("Bold")));
        titleLabel.setJustificationType(juce::Justification::centred);
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd5d8e2));
        addAndMakeVisible(titleLabel);
    }

    void attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float rulerY = bounds.getBottom() - 20.0f;
        const float trackY = bounds.getY() + 38.0f;
        const float leftX = bounds.getX() + 18.0f;
        const float rightX = bounds.getRight() - 18.0f;
        const float trackW = rightX - leftX;

        // Custom Slider Track
        g.setColour(juce::Colour(0xff121316));
        g.fillRoundedRectangle(leftX, trackY - 3.5f, trackW, 7.0f, 3.5f);
        g.setColour(juce::Colour(0xff252730));
        g.drawRoundedRectangle(leftX, trackY - 3.5f, trackW, 7.0f, 3.5f, 1.0f);

        // Active center fill
        const float centreTrackX = leftX + trackW * 0.5f;
        const float valNorm = static_cast<float>(slider.getValue() / 100.0);
        const float activeW = valNorm * (trackW * 0.45f);

        g.setColour(juce::Colour(0x60ffffff));
        g.fillRoundedRectangle(centreTrackX - activeW, trackY - 2.5f, activeW * 2.0f, 5.0f, 2.5f);

        // Thumb nubs (as seen in reference dual range appearance)
        const float thumbX1 = centreTrackX - activeW;
        const float thumbX2 = centreTrackX + activeW;

        g.setColour(juce::Colour(0xffffffff));
        g.fillRoundedRectangle(thumbX1 - 3.5f, trackY - 8.0f, 7.0f, 16.0f, 3.5f);
        g.fillRoundedRectangle(thumbX2 - 3.5f, trackY - 8.0f, 7.0f, 16.0f, 3.5f);

        // Scale markings below: -100, -50, 0, +50, +100
        struct ScaleMark { float val; const char* txt; };
        static const ScaleMark marks[] = {
            { -100.0f, "-100" },
            {  -50.0f,  "-50" },
            {    0.0f,    "0" },
            {  +50.0f,  "+50" },
            { +100.0f, "+100" }
        };

        g.setColour(juce::Colour(0x35ffffff));
        g.drawHorizontalLine(static_cast<int>(rulerY), leftX, rightX);

        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        for (const auto& m : marks)
        {
            const float norm = (m.val + 100.0f) / 200.0f;
            const float x = leftX + norm * trackW;

            g.setColour(juce::Colour(0x60ffffff));
            g.drawVerticalLine(static_cast<int>(x), rulerY - 3.0f, rulerY + 4.0f);

            g.setColour(juce::Colour(0xff7a7d88));
            g.drawText(m.txt, static_cast<int>(x - 20.0f), static_cast<int>(rulerY + 4.0f), 40, 14, juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        titleLabel.setBounds(bounds.removeFromTop(20));

        // Slider occupies track area
        auto trackArea = bounds.removeFromTop(36);
        slider.setBounds(trackArea.reduced(14, 0));
        slider.setAlpha(0.0f); // Make default JUCE track invisible, custom painted
    }

    juce::Slider& getSlider() noexcept { return slider; }

private:
    juce::Slider slider;
    juce::Label titleLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

} // namespace toytune
