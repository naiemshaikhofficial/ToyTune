#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/ToyTuneParameters.h"
#include "PitchCorrection/PitchCorrectionEngine.h"
#include "PitchCorrection/ScaleEngine.h"
#include "DSP/AudioTelemetry.h"
#include "Presets/PresetManager.h"

namespace toytune
{

class ToyTuneAudioProcessor : public juce::AudioProcessor
{
public:
    ToyTuneAudioProcessor();
    ~ToyTuneAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    AudioTelemetry& getTelemetry() noexcept { return telemetry; }
    ScaleEngine& getScaleEngine() noexcept { return scaleEngine; }
    PresetManager& getPresetManager() noexcept { return *presetManager; }

private:
    juce::AudioProcessorValueTreeState apvts;
    ScaleEngine scaleEngine;
    PitchCorrectionEngine engine;
    AudioTelemetry telemetry;
    std::unique_ptr<PresetManager> presetManager;

    void updateScaleEngineFromParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToyTuneAudioProcessor)
};

} // namespace toytune
