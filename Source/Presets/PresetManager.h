#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <vector>

namespace toytune
{

struct Preset
{
    juce::String name;
    float retuneSpeed { 20.0f };
    float strength { 100.0f };
    float tolerance { 0.0f };
    float humanize { 0.0f };
    float flexTune { 0.0f };
    float vibratoPreserve { 50.0f };
    int tuningMode { 1 }; // Modern
    int rootNote { 0 }; // C
    int scaleType { 0 }; // Chromatic
    bool formantPreserve { true };
    float formantShift { 0.0f };
};

class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    const std::vector<Preset>& getFactoryPresets() const noexcept { return factoryPresets; }
    juce::StringArray getPresetNames() const;

    void applyPreset(int index);
    void applyPreset(const juce::String& name);

    bool saveUserPreset(const juce::String& name);
    bool loadUserPreset(const juce::File& file);

    int getCurrentPresetIndex() const noexcept { return currentPresetIndex; }

private:
    juce::AudioProcessorValueTreeState& state;
    std::vector<Preset> factoryPresets;
    int currentPresetIndex { 1 }; // Modern Pop default

    void initFactoryPresets();
    void setParameterValue(const char* paramId, float value);
};

} // namespace toytune
