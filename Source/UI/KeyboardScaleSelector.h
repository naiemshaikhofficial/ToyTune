#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ToyTuneParameters.h"
#include <array>

namespace toytune
{

/**
 * Chromatic Note Selector Bar matching Producer Toy ToyTune reference image.
 * Features 12 rounded oval capsule buttons: C, C#, D, D#, E, F, F#, G, G#, A, A#, B.
 * Active sung note is highlighted with a bright white pill/halo as shown in reference.
 * Clicking a note toggles its inclusion in the active musical scale.
 */
class KeyboardScaleSelector : public juce::Component
{
public:
    explicit KeyboardScaleSelector(juce::AudioProcessorValueTreeState& state)
        : apvts(state)
    {
        for (int i = 0; i < 12; ++i)
        {
            noteButtons[i] = std::make_unique<juce::TextButton>(ToyTuneParameters::getRootNoteNames()[i]);
            noteButtons[i]->setClickingTogglesState(true);
            noteButtons[i]->setConnectedEdges(0);

            const auto paramId = ParameterIDs::noteMask(i);
            attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, *noteButtons[i]);
            addAndMakeVisible(*noteButtons[i]);
        }
    }

    void setActiveNoteDetected(int semitone)
    {
        if (currentDetectedSemitone != semitone)
        {
            currentDetectedSemitone = semitone;
            repaint();
        }
    }

    void updateScaleHighlights(const std::array<bool, 12>& mask)
    {
        currentMask = mask;
        repaint();
    }

    void paint(juce::Graphics& /*g*/) override
    {
        // Background handled per note button
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        for (int i = 0; i < 12; ++i)
        {
            const auto btnBounds = noteButtons[i]->getBounds().toFloat();
            const bool isSelectedInScale = noteButtons[i]->getToggleState();
            const bool isCurrentlySung = (currentDetectedSemitone == i);

            // Draw custom oval pill over the button
            if (isCurrentlySung)
            {
                // Active Sung Note: Bright white glowing pill with dark text
                g.setColour(juce::Colour(0x50ffffff));
                g.fillRoundedRectangle(btnBounds.expanded(2.0f), btnBounds.getHeight() * 0.5f);

                g.setColour(juce::Colour(0xffffffff));
                g.fillRoundedRectangle(btnBounds, btnBounds.getHeight() * 0.5f);

                g.setColour(juce::Colour(0xff0a0a0d));
                g.setFont(juce::Font(juce::FontOptions(12.5f).withStyle("Bold")));
                g.drawText(ToyTuneParameters::getRootNoteNames()[i], btnBounds, juce::Justification::centred, false);
            }
            else
            {
                // Normal note pill
                const juce::Colour bgCol = isSelectedInScale ? juce::Colour(0xff16171b) : juce::Colour(0xff0e0f12);
                const juce::Colour borderCol = isSelectedInScale ? juce::Colour(0xff2d3039) : juce::Colour(0xff1a1b20);
                const juce::Colour textCol = isSelectedInScale ? juce::Colour(0xffd5d8e2) : juce::Colour(0xff4a4c58);

                g.setColour(bgCol);
                g.fillRoundedRectangle(btnBounds, btnBounds.getHeight() * 0.5f);

                g.setColour(borderCol);
                g.drawRoundedRectangle(btnBounds, btnBounds.getHeight() * 0.5f, 1.2f);

                g.setColour(textCol);
                g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle(isSelectedInScale ? "Bold" : "Regular")));
                g.drawText(ToyTuneParameters::getRootNoteNames()[i], btnBounds, juce::Justification::centred, false);
            }
        }
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        const int numKeys = 12;
        const int buttonHeight = bounds.getHeight();
        const float gap = 6.0f;
        const float totalGaps = gap * static_cast<float>(numKeys - 1);
        const float buttonWidth = (static_cast<float>(bounds.getWidth()) - totalGaps) / static_cast<float>(numKeys);

        for (int i = 0; i < numKeys; ++i)
        {
            const float x = static_cast<float>(i) * (buttonWidth + gap);
            noteButtons[i]->setBounds(static_cast<int>(std::round(x)), 0,
                                      static_cast<int>(std::round(buttonWidth)), buttonHeight);
            noteButtons[i]->setAlpha(0.0f); // Make underlying JUCE button invisible, painted in paintOverChildren
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::array<std::unique_ptr<juce::TextButton>, 12> noteButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 12> attachments;
    std::array<bool, 12> currentMask;
    int currentDetectedSemitone { -1 };
};

} // namespace toytune
