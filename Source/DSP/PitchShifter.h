#pragma once

#include "CircularBuffer.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace toytune
{

/**
 * Authentic Pitch-Synchronous Resampling Pitch Shifter (Auto-Tune Architecture).
 * Uses a single primary resampling trajectory to eliminate flanging/comb filtering,
 * with pitch-period-locked in-phase synchronous splicing during boundary wraps.
 */
class PitchShifter
{
public:
    PitchShifter();
    ~PitchShifter() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setPitchRatioAndPitch(float ratio, float detectedPitchHz, bool isVoiced);
    void setPitchRatio(float ratio);
    void setLowLatencyMode(bool lowLatency);

    void process(juce::AudioBuffer<float>& buffer);

    int getLatencySamples() const noexcept { return latencySamples; }

private:
    double sampleRate { 44100.0 };
    float targetRatio { 1.0f };
    float smoothedRatio { 1.0f };

    bool isVoicedFrame { false };
    float fundamentalPeriod { 200.0f }; // T0 in samples

    bool isLowLatency { false };
    int latencySamples { 1024 };

    // High-capacity delay lines for left and right
    static constexpr int kBufferSize = 32768;
    std::vector<float> leftBuffer;
    std::vector<float> rightBuffer;
    int writePos { 0 };

    // Spliced Resampling State
    double primaryReadPos { 0.0 };
    double secondaryReadPos { 0.0 };
    bool isCrossfading { false };
    float xfadeProgress { 1.0f };
    float xfadeStep { 0.02f };

    inline float readCubic(const std::vector<float>& buf, double pos) const noexcept;
};

} // namespace toytune
