#include "PitchShifter.h"
#include <cmath>

namespace toytune
{

PitchShifter::PitchShifter()
{
    leftDelay.setSize(8192);
    rightDelay.setSize(8192);
}

void PitchShifter::prepare(double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 44100.0;
    updateWindowConfiguration();
    reset();
}

void PitchShifter::reset()
{
    leftDelay.reset();
    rightDelay.reset();
    phase1 = 0.0f;
    phase2 = 0.5f;
    smoothedRatio = targetRatio;
}

void PitchShifter::setPitchRatio(float ratio)
{
    targetRatio = juce::jlimit(0.25f, 4.0f, ratio);
}

void PitchShifter::setLowLatencyMode(bool lowLatency)
{
    if (isLowLatency != lowLatency)
    {
        isLowLatency = lowLatency;
        updateWindowConfiguration();
    }
}

void PitchShifter::updateWindowConfiguration()
{
    // High Quality: ~35 ms window; Low Latency: ~18 ms window
    const float windowDurationSec = isLowLatency ? 0.018f : 0.035f;
    windowSize = static_cast<int>(sampleRate * windowDurationSec);
    windowSize = juce::jlimit(256, 4096, (windowSize / 2) * 2); // Make even
    latencySamples = windowSize / 2;
}

inline float PitchShifter::calculateHannWindow(float phase) noexcept
{
    // Raised cosine / Hann window: 0.5 * (1 - cos(2 * pi * phase))
    return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    float* leftData = buffer.getWritePointer(0);
    float* rightData = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    const float windowSizeFloat = static_cast<float>(windowSize);
    const float ratioDeltaPerSample = (targetRatio - smoothedRatio) / static_cast<float>(numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        smoothedRatio += ratioDeltaPerSample;

        // Rate of change of delay: d(delay)/dt = 1 - ratio
        // Frequency shift rate per sample:
        const float phaseIncrement = (1.0f - smoothedRatio) / windowSizeFloat;

        phase1 += phaseIncrement;
        if (phase1 >= 1.0f)
            phase1 -= 1.0f;
        else if (phase1 < 0.0f)
            phase1 += 1.0f;

        phase2 += phaseIncrement;
        if (phase2 >= 1.0f)
            phase2 -= 1.0f;
        else if (phase2 < 0.0f)
            phase2 += 1.0f;

        // Current delay in samples for each read head
        const float delay1 = phase1 * windowSizeFloat;
        const float delay2 = phase2 * windowSizeFloat;

        // Crossfade gains
        const float gain1 = calculateHannWindow(phase1);
        const float gain2 = calculateHannWindow(phase2);

        // Normalize sum of squares / linear window sum
        const float totalGain = gain1 + gain2;
        const float norm1 = (totalGain > 1e-4f) ? (gain1 / totalGain) : 0.5f;
        const float norm2 = (totalGain > 1e-4f) ? (gain2 / totalGain) : 0.5f;

        // Process Left Channel
        const float inL = leftData[i];
        leftDelay.write(inL);
        const float shiftedL = (leftDelay.readCubic(delay1) * norm1) +
                               (leftDelay.readCubic(delay2) * norm2);
        leftData[i] = shiftedL;

        // Process Right Channel (stereo phase linked)
        if (rightData != nullptr)
        {
            const float inR = rightData[i];
            rightDelay.write(inR);
            const float shiftedR = (rightDelay.readCubic(delay1) * norm1) +
                                   (rightDelay.readCubic(delay2) * norm2);
            rightData[i] = shiftedR;
        }
    }
}

} // namespace toytune
