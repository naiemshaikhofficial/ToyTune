#pragma once

#include <juce_core/juce_core.h>
#include <atomic>

namespace toytune
{

/**
 * Thread-safe lock-free telemetry container passed from the real-time audio thread
 * to the GUI thread for visualization (pitch trace, cents deviation, meters).
 */
struct AudioTelemetry
{
    std::atomic<float> detectedPitchHz { 0.0f };
    std::atomic<float> detectedMidiNote { 0.0f };
    std::atomic<float> targetMidiNote { 0.0f };
    std::atomic<float> targetPitchHz { 0.0f };
    std::atomic<float> centsDeviation { 0.0f };
    std::atomic<float> pitchConfidence { 0.0f };
    std::atomic<bool>  isVoiced { false };
    
    std::atomic<float> inputRmsDb { -100.0f };
    std::atomic<float> outputRmsDb { -100.0f };
    std::atomic<float> currentRetuneRatio { 1.0f };

    // Ring buffer for scrolling pitch history visualization
    static constexpr int kHistoryCapacity = 512;
    float pitchHistory[kHistoryCapacity] = { 0.0f };
    float targetHistory[kHistoryCapacity] = { 0.0f };
    std::atomic<int> historyWritePos { 0 };

    void pushHistoryPoint(float detectedPitch, float targetPitch) noexcept
    {
        int writeIdx = historyWritePos.load(std::memory_order_relaxed);
        pitchHistory[writeIdx] = detectedPitch;
        targetHistory[writeIdx] = targetPitch;
        historyWritePos.store((writeIdx + 1) % kHistoryCapacity, std::memory_order_release);
    }
};

} // namespace toytune
