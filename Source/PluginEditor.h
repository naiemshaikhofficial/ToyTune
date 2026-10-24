#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/ToyTuneLookAndFeel.h"
#include "UI/HeaderBarComponent.h"
#include "UI/CentralPitchDisplayComponent.h"
#include "UI/KeyboardScaleSelector.h"
#include "UI/RotarySliderWithLabel.h"
#include "UI/SegmentedPillComponent.h"
#include "UI/PitchCorrectionRangeSlider.h"
#include "UI/AdvancedDrawerComponent.h"
#include "UI/HarmonyPlayerComponent.h"

namespace toytune
{

class ToyTuneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit ToyTuneAudioProcessorEditor(ToyTuneAudioProcessor&);
    ~ToyTuneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateTuningModeUI();
    void updateMainViewMode();

    ToyTuneAudioProcessor& audioProcessor;
    ToyTuneLookAndFeel customLookAndFeel;

    // 1. Top Header
    HeaderBarComponent headerBar;

    // 2. Upper Selectors Row
    juce::Label inputTypeLabel;
    std::unique_ptr<SegmentedPillComponent> inputTypeSelector;

    juce::Label keyScaleLabel;
    juce::ComboBox rootKeyBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment;
    juce::ComboBox scaleTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleTypeAttachment;

    juce::Label tuningModeLabel;
    std::unique_ptr<SegmentedPillComponent> tuningModeSelector;
    juce::TextButton harmonyModeToggleBtn;

    // 3. Middle Section
    RotarySliderWithLabel retuneSpeedKnob;
    CentralPitchDisplayComponent centralPitchDisplay;
    HarmonyPlayerComponent harmonyPlayer;
    RotarySliderWithLabel humanizeKnob;
    bool isHarmonyViewActive { false };

    // 4. Advanced Drawer & Chromatic Keyboard Bar
    AdvancedDrawerComponent advancedDrawer;
    KeyboardScaleSelector keyboardSelector;

    // 5. Bottom Control Section
    juce::Rectangle<int> bottomShelfBounds;
    RotarySliderWithLabel flexTuneKnob;
    RotarySliderWithLabel vibratoKnob;
    RotarySliderWithLabel formantKnob;
    PitchCorrectionRangeSlider rangeSlider;
    RotarySliderWithLabel mixKnob;
    RotarySliderWithLabel outputGainKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToyTuneAudioProcessorEditor)
};

} // namespace toytune
