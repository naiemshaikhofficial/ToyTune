#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/AudioTelemetry.h"
#include <vector>

namespace toytune
{

class PitchHistoryComponent : public juce::Component
{
public:
    explicit PitchHistoryComponent(AudioTelemetry& telemetry);
    ~PitchHistoryComponent() override = default;

    void updateTelemetry();
    void paint(juce::Graphics& g) override;

private:
    AudioTelemetry& audioTelemetry;

    std::vector<float> detectedPoints;
    std::vector<float> targetPoints;

    float minMidi { 45.0f }; // A2
    float maxMidi { 81.0f }; // A5

    float midiToY(float midiNote, float height) const noexcept;
};

} // namespace toytune
