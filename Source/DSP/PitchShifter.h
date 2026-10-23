#pragma once

#include "CircularBuffer.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace toytune
{

/**
 * Real-time stereo phase-locked pitch shifter using dual synchronized read heads
 * with smooth Hann crossfading and cubic fractional delay interpolation.
 */
class PitchShifter
{
public:
    PitchShifter();
    ~PitchShifter() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setPitchRatio(float ratio);
    void setLowLatencyMode(bool lowLatency);

    /**
     * Process audio buffer in-place. Left and right channels share the same
     * pitch transposition ratio and crossfade phases to preserve stereo imaging.
     */
    void process(juce::AudioBuffer<float>& buffer);

    int getLatencySamples() const noexcept { return latencySamples; }

private:
    double sampleRate { 44100.0 };
    float targetRatio { 1.0f };
    float smoothedRatio { 1.0f };

    bool isLowLatency { false };
    int windowSize { 1024 }; // Window length in samples
    int latencySamples { 512 };

    CircularBuffer leftDelay;
    CircularBuffer rightDelay;

    // Dual read-head phase pointers [0.0, 1.0)
    float phase1 { 0.0f };
    float phase2 { 0.5f };

    void updateWindowConfiguration();
    static inline float calculateHannWindow(float phase) noexcept;
};

} // namespace toytune
