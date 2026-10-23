#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/ToyTuneLookAndFeel.h"
#include "UI/HeaderBarComponent.h"
#include "UI/PitchLaneComponent.h"
#include "UI/KeyboardScaleSelector.h"
#include "UI/RotarySliderWithLabel.h"
#include "UI/LevelMeterComponent.h"

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

    ToyTuneAudioProcessor& audioProcessor;
    ToyTuneLookAndFeel customLookAndFeel;

    // Header
    HeaderBarComponent headerBar;

    // Central Pitch Workspace
    PitchLaneComponent pitchLane;

    // Key & Scale Section
    juce::GroupComponent scaleGroup;
    juce::Label rootLabel;
    juce::ComboBox rootKeyBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment;

    juce::Label scaleLabel;
    juce::ComboBox scaleTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleTypeAttachment;

    juce::Label refPitchLabel;
    juce::Slider refPitchSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> refPitchAttachment;

    KeyboardScaleSelector keyboardSelector;

    // Tuning Controls Section
    juce::GroupComponent tuningGroup;
    RotarySliderWithLabel retuneSpeedKnob;
    RotarySliderWithLabel correctionStrengthKnob;
    RotarySliderWithLabel pitchToleranceKnob;
    RotarySliderWithLabel humanizeKnob;
    RotarySliderWithLabel flexTuneKnob;
    RotarySliderWithLabel vibratoPreserveKnob;

    juce::Label modeLabel;
    juce::ComboBox tuningModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tuningModeAttachment;

    // Bottom Utility Strip
    juce::GroupComponent utilityGroup;
    LevelMeterComponent inMeter;
    LevelMeterComponent outMeter;

    RotarySliderWithLabel inputGainKnob;
    RotarySliderWithLabel outputGainKnob;
    RotarySliderWithLabel mixKnob;

    juce::Label vocalRangeLabel;
    juce::ComboBox vocalRangeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> vocalRangeAttachment;

    juce::ToggleButton formantPreserveBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> formantPreserveAttachment;

    RotarySliderWithLabel formantShiftKnob;

    juce::ToggleButton lowLatencyBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lowLatencyAttachment;

    juce::Label latencyDisplay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToyTuneAudioProcessorEditor)
};

} // namespace toytune
