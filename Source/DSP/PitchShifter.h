#pragma once

#include "CircularBuffer.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace toytune
{

/**
 * Professional Pitch-Synchronous (PSOLA / Period-Adaptive) real-time pitch shifter.
 * Based on Dr. Andy Hildebrand's foundational Auto-Tune principles (US Patent 5,973,252):
 * Dynamically locks the overlap-add crossfade window to the vocal fundamental period (W = 2 * T0),
 * maintaining constructive phase alignment between glottal pulses and eliminating comb-filtering
 * and chorusing artifacts.
 */
class PitchShifter
{
public:
    PitchShifter();
    ~PitchShifter() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    /**
     * Sets pitch transposition ratio and fundamental frequency for pitch-synchronous alignment.
     * @param ratio Multiplicative pitch shift ratio (e.g. 1.05946 for +1 semitone).
     * @param detectedPitchHz Detected fundamental pitch in Hz (0 if unvoiced).
     * @param isVoiced Whether current audio frame is voiced vocal.
     */
    void setPitchRatioAndPitch(float ratio, float detectedPitchHz, bool isVoiced);

    void setPitchRatio(float ratio);
    void setLowLatencyMode(bool lowLatency);

    /**
     * Process audio buffer in-place. Left and right channels share the exact same
     * pitch-synchronous phase and period modulation to preserve perfect stereo imaging.
     */
    void process(juce::AudioBuffer<float>& buffer);

    int getLatencySamples() const noexcept { return latencySamples; }

private:
    double sampleRate { 44100.0 };
    float targetRatio { 1.0f };
    float smoothedRatio { 1.0f };

    bool isVoicedFrame { false };
    float currentPitchHz { 220.0f };
    float targetWindowSize { 400.0f };
    float currentWindowSize { 400.0f };

    bool isLowLatency { false };
    int latencySamples { 256 };

    CircularBuffer leftDelay;
    CircularBuffer rightDelay;

    // Dual read-head phase pointers [0.0, 1.0)
    float phase1 { 0.0f };
    float phase2 { 0.5f };

    void updateWindowConfiguration();
    static inline float calculateHannWindow(float phase) noexcept;
};

} // namespace toytune
