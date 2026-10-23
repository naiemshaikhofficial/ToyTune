#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include "../Parameters/ToyTuneParameters.h"

namespace toytune
{

struct PitchDetectionResult
{
    float pitchHz { 0.0f };
    float midiNote { 0.0f };
    float confidence { 0.0f };
    bool isVoiced { false };
    float rmsDb { -100.0f };
};

/**
 * High-performance, low-latency monophonic vocal F0 detector based on
 * Cumulative Mean Normalized Difference (CMNDF / YIN) with sub-sample
 * parabolic interpolation, octave jump rejection, and voiced/unvoiced gating.
 */
class PitchDetector
{
public:
    PitchDetector();
    ~PitchDetector() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setVocalRange(VocalRangeType rangeType);
    void setReferencePitch(float refHz);

    /**
     * Process an incoming audio block (mono or downmixed) and estimate F0.
     * Guaranteed no heap allocations during audio processing.
     */
    PitchDetectionResult processBlock(const float* channelData, int numSamples);

    float getCurrentPitchHz() const noexcept { return currentPitchHz; }
    float getCurrentMidiNote() const noexcept { return currentMidiNote; }
    float getConfidence() const noexcept { return currentConfidence; }
    bool isCurrentlyVoiced() const noexcept { return isVoiced; }

private:
    double sampleRate { 44100.0 };
    float referenceA4 { 440.0f };
    VocalRangeType vocalRange { VocalRangeType::Wide };

    int minPeriodSamples { 40 };   // ~1100 Hz at 44.1 kHz
    int maxPeriodSamples { 800 };  // ~55 Hz at 44.1 kHz

    // Internal analysis circular buffer (Power of 2 for fast, safe bitwise masking)
    static constexpr int kBufferSize = 8192;
    std::vector<float> inputBuffer;
    int writeIndex { 0 };

    // Work buffers pre-allocated to avoid audio-thread heap allocation
    std::vector<float> differenceBuffer;
    std::vector<float> cmndfBuffer;
    std::vector<float> linearBuffer;

    float currentPitchHz { 0.0f };
    float currentMidiNote { 0.0f };
    float currentConfidence { 0.0f };
    bool isVoiced { false };

    // Octave rejection tracking
    float previousPeriod { 0.0f };
    int unvoicedFramesCount { 0 };

    void updatePeriodLimits();
    float calculateRmsDb(const float* data, int numSamples) const noexcept;
    float frequencyToMidi(float freqHz) const noexcept;
};

} // namespace toytune
