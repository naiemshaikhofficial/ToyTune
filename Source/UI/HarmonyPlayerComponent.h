#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ToyTuneParameters.h"
#include "RotarySliderWithLabel.h"
#include <array>

namespace toytune
{

/**
 * Auto-Tune Pro 11-Grade Harmony Player Component.
 * Features:
 * - 4 Dedicated Harmony Voice Channels in 2x2 grid (Voice 1, 2, 3, 4)
 * - Per-voice Diatonic Interval selection, Formant gender shift, Constant-Power Pan, Level dB, and Solo
 * - Central Humanization strip (Naturalize, Transition Time, Pitch Variation, Timing Variation)
 * - Mastering & Dynamics rack (Attack, Release, High-Pass Filter, Stereo Width)
 * Styled in ToyTune dark luxury monochrome aesthetic with glowing white indicators.
 */
class HarmonyPlayerComponent : public juce::Component
{
public:
    explicit HarmonyPlayerComponent(juce::AudioProcessorValueTreeState& state)
        : apvts(state),
          naturalizeKnob("Naturalize", "%", false),
          transitionKnob("Transition", "ms", false),
          pitchVarKnob("Pitch Var", "ct", false),
          timingVarKnob("Timing Var", "ms", false),
          attackKnob("Attack", "ms", false),
          releaseKnob("Release", "ms", false),
          eqKnob("High-Pass", "Hz", false),
          widthKnob("Width", "%", false)
    {
        // 1. Top Bar Mode Selector
        harmonyModeBox.addItemList(juce::StringArray { "Scale Diatonic", "Fixed Semitones", "MIDI Notes" }, 1);
        harmonyModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParameterIDs::harmonyMode, harmonyModeBox);
        addAndMakeVisible(harmonyModeBox);

        triggerAllBtn.setButtonText("Trigger All");
        addAndMakeVisible(triggerAllBtn);

        // 2. 4 Harmony Voice Channels
        static const juce::StringArray intervalNames {
            "-12 (Oct Down)", "-10 (7th Down)", "-7 (5th Down)", "-5 (4th Down)", "-3/-4 (3rd Down)",
            "0 (Unison)",
            "+3/+4 (3rd Up)", "+5 (4th Up)", "+7 (5th Up)", "+10 (7th Up)", "+12 (Oct Up)"
        };

        for (int i = 0; i < 4; ++i)
        {
            // Enable toggle
            voiceEnableButtons[i] = std::make_unique<juce::TextButton>("Voice " + juce::String(i + 1));
            voiceEnableButtons[i]->setClickingTogglesState(true);
            voiceEnableAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, ParameterIDs::harmVoiceEnable(i), *voiceEnableButtons[i]);
            addAndMakeVisible(*voiceEnableButtons[i]);

            // Solo toggle
            voiceSoloButtons[i] = std::make_unique<juce::TextButton>("[S]");
            voiceSoloButtons[i]->setClickingTogglesState(true);
            voiceSoloAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, ParameterIDs::harmVoiceSolo(i), *voiceSoloButtons[i]);
            addAndMakeVisible(*voiceSoloButtons[i]);

            // Interval ComboBox
            voiceIntervalBoxes[i] = std::make_unique<juce::ComboBox>();
            voiceIntervalBoxes[i]->addItemList(intervalNames, 1);
            voiceIntervalAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, ParameterIDs::harmVoiceInterval(i), *voiceIntervalBoxes[i]);
            addAndMakeVisible(*voiceIntervalBoxes[i]);

            // Formant, Pan, Level Knobs
            voiceFormantKnobs[i] = std::make_unique<RotarySliderWithLabel>("Formant", "st", true);
            voiceFormantKnobs[i]->attachToParameter(apvts, ParameterIDs::harmVoiceFormant(i));
            addAndMakeVisible(*voiceFormantKnobs[i]);

            voicePanKnobs[i] = std::make_unique<RotarySliderWithLabel>("Pan", "", true);
            voicePanKnobs[i]->attachToParameter(apvts, ParameterIDs::harmVoicePan(i));
            addAndMakeVisible(*voicePanKnobs[i]);

            voiceLevelKnobs[i] = std::make_unique<RotarySliderWithLabel>("Level", "dB", true);
            voiceLevelKnobs[i]->attachToParameter(apvts, ParameterIDs::harmVoiceLevel(i));
            addAndMakeVisible(*voiceLevelKnobs[i]);
        }

        // 3. Humanize Knobs
        naturalizeKnob.attachToParameter(apvts, ParameterIDs::harmNaturalize);
        addAndMakeVisible(naturalizeKnob);

        transitionKnob.attachToParameter(apvts, ParameterIDs::harmTransition);
        addAndMakeVisible(transitionKnob);

        pitchVarKnob.attachToParameter(apvts, ParameterIDs::harmPitchVar);
        addAndMakeVisible(pitchVarKnob);

        timingVarKnob.attachToParameter(apvts, ParameterIDs::harmTimingVar);
        addAndMakeVisible(timingVarKnob);

        // 4. Mastering Strip Knobs
        attackKnob.attachToParameter(apvts, ParameterIDs::harmEnvAttack);
        addAndMakeVisible(attackKnob);

        releaseKnob.attachToParameter(apvts, ParameterIDs::harmEnvRelease);
        addAndMakeVisible(releaseKnob);

        eqKnob.attachToParameter(apvts, ParameterIDs::harmEqFreq);
        addAndMakeVisible(eqKnob);

        widthKnob.attachToParameter(apvts, ParameterIDs::harmStereoWidth);
        addAndMakeVisible(widthKnob);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Overall dark container
        g.setColour(juce::Colour(0xff0e0f12));
        g.fillRoundedRectangle(bounds, 10.0f);
        g.setColour(juce::Colour(0xff1e2027));
        g.drawRoundedRectangle(bounds, 10.0f, 1.2f);

        // Header bar label
        g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        g.setColour(juce::Colour(0xff8a8d9a));
        g.drawText("HARMONY INTERVAL MODE", 16, 10, 160, 20, juce::Justification::centredLeft, false);

        // Draw 4 Voice Card Backgrounds
        for (int i = 0; i < 4; ++i)
        {
            if (voiceCards[i].getWidth() > 0)
            {
                const auto card = voiceCards[i].toFloat();
                const bool isEnabled = voiceEnableButtons[i]->getToggleState();

                g.setColour(isEnabled ? juce::Colour(0xff121317) : juce::Colour(0xff0b0c0e));
                g.fillRoundedRectangle(card, 8.0f);

                g.setColour(isEnabled ? juce::Colour(0xff22252e) : juce::Colour(0xff16171b));
                g.drawRoundedRectangle(card, 8.0f, 1.0f);
            }
        }

        // Draw Humanize Section Card
        if (humanizeCard.getWidth() > 0)
        {
            const auto card = humanizeCard.toFloat();
            g.setColour(juce::Colour(0xff121317));
            g.fillRoundedRectangle(card, 8.0f);
            g.setColour(juce::Colour(0xff22252e));
            g.drawRoundedRectangle(card, 8.0f, 1.0f);

            g.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff8a8d9a));
            g.drawText("HUMANIZE", card.withTrimmedTop(6.0f).withHeight(16.0f), juce::Justification::centred, false);
        }

        // Draw Mastering Section Card
        if (masteringCard.getWidth() > 0)
        {
            const auto card = masteringCard.toFloat();
            g.setColour(juce::Colour(0xff121317));
            g.fillRoundedRectangle(card, 8.0f);
            g.setColour(juce::Colour(0xff22252e));
            g.drawRoundedRectangle(card, 8.0f, 1.0f);

            g.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff8a8d9a));
            g.drawText("MASTERING & EQ", card.withTrimmedTop(6.0f).withHeight(16.0f), juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        const auto bounds = getLocalBounds().reduced(12, 10);
        const int W = bounds.getWidth();
        juce::ignoreUnused(bounds.getHeight());

        // 1. Top Header Row (Mode Selector & Trigger All)
        const int topH = 28;
        harmonyModeBox.setBounds(bounds.getX() + 160, bounds.getY() + 2, 150, topH);
        triggerAllBtn.setBounds(bounds.getRight() - 110, bounds.getY() + 2, 100, topH);

        // Available Area below top row
        auto contentArea = bounds.withTrimmedTop(topH + 10);

        // Partition columns:
        // Left Area: 2x2 Voice Matrix (~60% width)
        // Center-Right: Humanize Column (~20% width)
        // Far-Right: Mastering Column (~20% width)
        const int rightColW = juce::jlimit(130, 160, static_cast<int>(W * 0.19f));
        const int midColW = juce::jlimit(130, 160, static_cast<int>(W * 0.19f));
        const int voiceMatrixW = contentArea.getWidth() - midColW - rightColW - 20;

        auto voiceArea = contentArea.removeFromLeft(voiceMatrixW);
        contentArea.removeFromLeft(10);
        humanizeCard = contentArea.removeFromLeft(midColW);
        contentArea.removeFromLeft(10);
        masteringCard = contentArea;

        // Layout 2x2 Voice Cards
        const int cardGap = 8;
        const int cardW = (voiceArea.getWidth() - cardGap) / 2;
        const int cardH = (voiceArea.getHeight() - cardGap) / 2;

        voiceCards[0] = juce::Rectangle<int>(voiceArea.getX(), voiceArea.getY(), cardW, cardH);
        voiceCards[1] = juce::Rectangle<int>(voiceArea.getX() + cardW + cardGap, voiceArea.getY(), cardW, cardH);
        voiceCards[2] = juce::Rectangle<int>(voiceArea.getX(), voiceArea.getY() + cardH + cardGap, cardW, cardH);
        voiceCards[3] = juce::Rectangle<int>(voiceArea.getX() + cardW + cardGap, voiceArea.getY() + cardH + cardGap, cardW, cardH);

        for (int i = 0; i < 4; ++i)
        {
            auto card = voiceCards[i].reduced(8, 6);
            const int btnH = 22;

            // Top row inside card: [Voice 1] [Interval Box] [Solo]
            const int soloW = 28;
            const int enableW = 68;
            const int intervalW = card.getWidth() - soloW - enableW - 8;

            voiceEnableButtons[i]->setBounds(card.getX(), card.getY(), enableW, btnH);
            voiceIntervalBoxes[i]->setBounds(card.getX() + enableW + 4, card.getY(), intervalW, btnH);
            voiceSoloButtons[i]->setBounds(card.getX() + enableW + 4 + intervalW + 4, card.getY(), soloW, btnH);

            // Knobs row: Formant, Pan, Level
            const int knobY = card.getY() + btnH + 4;
            const int knobH = card.getBottom() - knobY;
            const int knobW = card.getWidth() / 3;

            voiceFormantKnobs[i]->setBounds(card.getX(), knobY, knobW, knobH);
            voicePanKnobs[i]->setBounds(card.getX() + knobW, knobY, knobW, knobH);
            voiceLevelKnobs[i]->setBounds(card.getX() + 2 * knobW, knobY, knobW, knobH);
        }

        // Layout Humanize Knobs (4 vertical)
        {
            auto hArea = humanizeCard.reduced(8, 26);
            const int knobH = hArea.getHeight() / 4;
            naturalizeKnob.setBounds(hArea.getX(), hArea.getY(), hArea.getWidth(), knobH);
            transitionKnob.setBounds(hArea.getX(), hArea.getY() + knobH, hArea.getWidth(), knobH);
            pitchVarKnob.setBounds(hArea.getX(), hArea.getY() + 2 * knobH, hArea.getWidth(), knobH);
            timingVarKnob.setBounds(hArea.getX(), hArea.getY() + 3 * knobH, hArea.getWidth(), knobH);
        }

        // Layout Mastering Knobs (4 vertical)
        {
            auto mArea = masteringCard.reduced(8, 26);
            const int knobH = mArea.getHeight() / 4;
            attackKnob.setBounds(mArea.getX(), mArea.getY(), mArea.getWidth(), knobH);
            releaseKnob.setBounds(mArea.getX(), mArea.getY() + knobH, mArea.getWidth(), knobH);
            eqKnob.setBounds(mArea.getX(), mArea.getY() + 2 * knobH, mArea.getWidth(), knobH);
            widthKnob.setBounds(mArea.getX(), mArea.getY() + 3 * knobH, mArea.getWidth(), knobH);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::ComboBox harmonyModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> harmonyModeAttachment;
    juce::TextButton triggerAllBtn;

    std::array<juce::Rectangle<int>, 4> voiceCards;
    juce::Rectangle<int> humanizeCard;
    juce::Rectangle<int> masteringCard;

    std::array<std::unique_ptr<juce::TextButton>, 4> voiceEnableButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 4> voiceEnableAttachments;

    std::array<std::unique_ptr<juce::TextButton>, 4> voiceSoloButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 4> voiceSoloAttachments;

    std::array<std::unique_ptr<juce::ComboBox>, 4> voiceIntervalBoxes;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 4> voiceIntervalAttachments;

    std::array<std::unique_ptr<RotarySliderWithLabel>, 4> voiceFormantKnobs;
    std::array<std::unique_ptr<RotarySliderWithLabel>, 4> voicePanKnobs;
    std::array<std::unique_ptr<RotarySliderWithLabel>, 4> voiceLevelKnobs;

    RotarySliderWithLabel naturalizeKnob;
    RotarySliderWithLabel transitionKnob;
    RotarySliderWithLabel pitchVarKnob;
    RotarySliderWithLabel timingVarKnob;

    RotarySliderWithLabel attackKnob;
    RotarySliderWithLabel releaseKnob;
    RotarySliderWithLabel eqKnob;
    RotarySliderWithLabel widthKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonyPlayerComponent)
};

} // namespace toytune
