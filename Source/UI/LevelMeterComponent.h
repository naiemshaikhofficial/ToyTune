#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace toytune
{

class LevelMeterComponent : public juce::Component
{
public:
    explicit LevelMeterComponent(const juce::String& labelText)
        : title(labelText)
    {
    }

    void setLevel(float rmsDb)
    {
        currentDb = juce::jlimit(-60.0f, 6.0f, rmsDb);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Meter Title
        g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
        g.setColour(juce::Colour(0xff7a7a85));
        g.drawText(title, 0, 0, static_cast<int>(bounds.getWidth()), 12, juce::Justification::centred);

        auto meterBarArea = bounds;
        meterBarArea.removeFromTop(14.0f);

        // Meter background channel
        g.setColour(juce::Colour(0xff0d0d10));
        g.fillRoundedRectangle(meterBarArea, 2.0f);
        g.setColour(juce::Colour(0xff1f1f26));
        g.drawRoundedRectangle(meterBarArea, 2.0f, 1.0f);

        // Normalised height [0, 1] from -60 dB to 0 dB
        const float norm = juce::jlimit(0.0f, 1.0f, (currentDb + 60.0f) / 60.0f);
        if (norm > 0.01f)
        {
            const float barH = norm * meterBarArea.getHeight();
            const auto barRect = juce::Rectangle<float>(
                meterBarArea.getX() + 1.5f,
                meterBarArea.getBottom() - barH - 1.0f,
                meterBarArea.getWidth() - 3.0f,
                barH);

            juce::ColourGradient grad(
                juce::Colour(0xffe6e6ec), barRect.getCentreX(), barRect.getY(),
                juce::Colour(0xff707078), barRect.getCentreX(), barRect.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(barRect, 1.5f);
        }
    }

private:
    juce::String title;
    float currentDb { -60.0f };
};

} // namespace toytune
