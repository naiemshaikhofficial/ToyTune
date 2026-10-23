#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ToyTuneParameters.h"
#include <array>

namespace toytune
{

class KeyboardScaleSelector : public juce::Component
{
public:
    KeyboardScaleSelector(juce::AudioProcessorValueTreeState& state)
        : apvts(state)
    {
        for (int i = 0; i < 12; ++i)
        {
            noteButtons[i] = std::make_unique<juce::TextButton>(ToyTuneParameters::getRootNoteNames()[i]);
            noteButtons[i]->setClickingTogglesState(true);
            noteButtons[i]->setColour(juce::TextButton::buttonColourId, isBlackKey(i) ? juce::Colour(0xff121215) : juce::Colour(0xff1f1f26));
            noteButtons[i]->setColour(juce::TextButton::buttonOnColourId, isBlackKey(i) ? juce::Colour(0xff2a2a34) : juce::Colour(0xff363642));
            noteButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colour(0xff555560));
            noteButtons[i]->setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));

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
        for (int i = 0; i < 12; ++i)
        {
            noteButtons[i]->setAlpha(currentMask[i] ? 1.0f : 0.45f);
        }
        repaint();
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        if (currentDetectedSemitone >= 0 && currentDetectedSemitone < 12)
        {
            const auto btnBounds = noteButtons[currentDetectedSemitone]->getBounds().toFloat();
            g.setColour(juce::Colour(0xffffffff));
            g.drawRoundedRectangle(btnBounds.reduced(1.0f), 3.0f, 1.5f);

            // Small indicator pip
            g.fillEllipse(btnBounds.getCentreX() - 2.5f, btnBounds.getBottom() - 7.0f, 5.0f, 5.0f);
        }
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        const int numKeys = 12;
        const int keyWidth = bounds.getWidth() / numKeys;
        const int keyHeight = bounds.getHeight();

        for (int i = 0; i < numKeys; ++i)
        {
            noteButtons[i]->setBounds(i * keyWidth, 0, keyWidth - 2, keyHeight);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::array<std::unique_ptr<juce::TextButton>, 12> noteButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 12> attachments;
    std::array<bool, 12> currentMask;
    int currentDetectedSemitone { -1 };

    static bool isBlackKey(int semitone) noexcept
    {
        // C=0, C#=1, D=2, D#=3, E=4, F=5, F#=6, G=7, G#=8, A=9, A#=10, B=11
        return (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10);
    }
};

} // namespace toytune
