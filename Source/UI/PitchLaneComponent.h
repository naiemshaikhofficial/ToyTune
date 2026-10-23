#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PitchHistoryComponent.h"
#include "../DSP/AudioTelemetry.h"
#include "../PitchCorrection/ScaleEngine.h"

namespace toytune
{

class PitchLaneComponent : public juce::Component
{
public:
    explicit PitchLaneComponent(AudioTelemetry& telemetry)
        : audioTelemetry(telemetry),
          historyComp(telemetry)
    {
        addAndMakeVisible(historyComp);
    }

    void updateTelemetry()
    {
        historyComp.updateTelemetry();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Panel backdrop
        g.setColour(juce::Colour(0xff121215));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(juce::Colour(0xff222228));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        // Header strip inside pitch lane
        auto headerArea = bounds.removeFromTop(36.0f);

        const float detHz = audioTelemetry.detectedPitchHz.load(std::memory_order_relaxed);
        const float detMidi = audioTelemetry.detectedMidiNote.load(std::memory_order_relaxed);
        const float targetMidi = audioTelemetry.targetMidiNote.load(std::memory_order_relaxed);
        const float cents = audioTelemetry.centsDeviation.load(std::memory_order_relaxed);
        const float conf = audioTelemetry.pitchConfidence.load(std::memory_order_relaxed);
        juce::ignoreUnused(conf);
        const bool voiced = audioTelemetry.isVoiced.load(std::memory_order_relaxed);

        // Left: Detected Note Badge
        juce::String noteStr = "--";
        if (voiced && detMidi > 10.0f)
            noteStr = ScaleEngine::getNoteNameWithOctave(static_cast<int>(std::round(detMidi)));

        auto badgeArea = headerArea.removeFromLeft(90.0f).reduced(4.0f);
        g.setColour(juce::Colour(0xff1a1a20));
        g.fillRoundedRectangle(badgeArea, 4.0f);
        g.setColour(juce::Colour(0xff2e2e38));
        g.drawRoundedRectangle(badgeArea, 4.0f, 1.0f);

        g.setColour(voiced ? juce::Colour(0xffffffff) : juce::Colour(0xff6a6a75));
        g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
        g.drawFittedText(noteStr, badgeArea.toNearestInt(), juce::Justification::centred, 1);

        // Frequency display
        auto freqArea = headerArea.removeFromLeft(90.0f);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.setColour(juce::Colour(0xff8a8a95));
        const juce::String freqStr = voiced ? (juce::String(detHz, 1) + " Hz") : "Unvoiced";
        g.drawFittedText(freqStr, freqArea.toNearestInt(), juce::Justification::centredLeft, 1);

        // Center: Cents deviation meter
        auto centsArea = headerArea.removeFromLeft(220.0f).reduced(4.0f, 6.0f);
        g.setColour(juce::Colour(0xff0d0d10));
        g.fillRoundedRectangle(centsArea, 3.0f);
        g.setColour(juce::Colour(0xff222228));
        g.drawRoundedRectangle(centsArea, 3.0f, 1.0f);

        const float midX = centsArea.getCentreX();
        g.setColour(juce::Colour(0x40ffffff));
        g.drawVerticalLine(static_cast<int>(midX), centsArea.getY(), centsArea.getBottom());

        if (voiced && std::abs(cents) <= 50.0f)
        {
            const float deviationNorm = cents / 50.0f; // -1 to +1
            const float barW = deviationNorm * (centsArea.getWidth() * 0.45f);

            juce::Rectangle<float> barRect;
            if (barW >= 0.0f)
                barRect = juce::Rectangle<float>(midX, centsArea.getY() + 3.0f, barW, centsArea.getHeight() - 6.0f);
            else
                barRect = juce::Rectangle<float>(midX + barW, centsArea.getY() + 3.0f, -barW, centsArea.getHeight() - 6.0f);

            g.setColour(juce::Colour(0xffe0e0e6));
            g.fillRoundedRectangle(barRect, 2.0f);
        }

        // Cents text indicator
        auto centsValArea = headerArea.removeFromLeft(80.0f);
        g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        g.setColour(juce::Colour(0xffc5c5cb));
        const juce::String centsStr = voiced ? ((cents >= 0.0f ? "+" : "") + juce::String(cents, 1) + " ct") : "0 ct";
        g.drawFittedText(centsStr, centsValArea.toNearestInt(), juce::Justification::centredLeft, 1);

        // Right: Target Note Badge
        juce::String tgtStr = "--";
        if (voiced && targetMidi > 10.0f)
            tgtStr = "Target: " + ScaleEngine::getNoteNameWithOctave(static_cast<int>(std::round(targetMidi)));

        auto tgtArea = headerArea.removeFromRight(120.0f);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.setColour(juce::Colour(0xff9a9aa5));
        g.drawFittedText(tgtStr, tgtArea.toNearestInt(), juce::Justification::centredRight, 1);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(38); // Header
        historyComp.setBounds(bounds.reduced(4));
    }

private:
    AudioTelemetry& audioTelemetry;
    PitchHistoryComponent historyComp;
};

} // namespace toytune
