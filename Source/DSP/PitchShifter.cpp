#include "PitchShifter.h"
#include <cmath>
#include <algorithm>

namespace toytune
{

PitchShifter::PitchShifter()
{
    leftBuffer.assign(kBufferSize, 0.0f);
    rightBuffer.assign(kBufferSize, 0.0f);
}

void PitchShifter::prepare(double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 44100.0;
    latencySamples = isLowLatency ? 512 : 1024;
    reset();
}

void PitchShifter::reset()
{
    std::fill(leftBuffer.begin(), leftBuffer.end(), 0.0f);
    std::fill(rightBuffer.begin(), rightBuffer.end(), 0.0f);
    writePos = 0;
    
    // Set nominal delay
    const double initialDelay = static_cast<double>(latencySamples);
    primaryReadPos = static_cast<double>(kBufferSize) - initialDelay;
    secondaryReadPos = primaryReadPos;
    isCrossfading = false;
    xfadeProgress = 1.0f;
    smoothedRatio = targetRatio;
}

void PitchShifter::setPitchRatioAndPitch(float ratio, float detectedPitchHz, bool isVoiced)
{
    targetRatio = juce::jlimit(0.25f, 4.0f, ratio);
    isVoicedFrame = isVoiced;

    if (isVoiced && detectedPitchHz >= 45.0f && detectedPitchHz <= 1200.0f)
    {
        fundamentalPeriod = juce::jlimit(30.0f, 1000.0f, static_cast<float>(sampleRate / detectedPitchHz));
    }
    else
    {
        fundamentalPeriod = static_cast<float>(sampleRate / 220.0); // Default ~200 samples
    }
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
        latencySamples = isLowLatency ? 512 : 1024;
        reset();
    }
}

inline float PitchShifter::readCubic(const std::vector<float>& buf, double pos) const noexcept
{
    if (!std::isfinite(pos))
        return 0.0f;

    pos = std::fmod(pos, static_cast<double>(kBufferSize));
    if (pos < 0.0)
        pos += static_cast<double>(kBufferSize);

    const int i1 = static_cast<int>(pos) & (kBufferSize - 1);
    const float frac = static_cast<float>(pos - static_cast<double>(i1));

    const int i0 = (i1 - 1) & (kBufferSize - 1);
    const int i2 = (i1 + 1) & (kBufferSize - 1);
    const int i3 = (i1 + 2) & (kBufferSize - 1);

    const float y0 = buf[static_cast<size_t>(i0)];
    const float y1 = buf[static_cast<size_t>(i1)];
    const float y2 = buf[static_cast<size_t>(i2)];
    const float y3 = buf[static_cast<size_t>(i3)];

    // 4-point Hermite polynomial interpolation
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    float* leftData = buffer.getWritePointer(0);
    float* rightData = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    const float ratioDelta = (targetRatio - smoothedRatio) / static_cast<float>(numSamples);

    const float T0 = fundamentalPeriod;
    const double nominalDelay = static_cast<double>(latencySamples);
    const double minDelay = std::max(64.0, nominalDelay * 0.25);
    const double maxDelay = nominalDelay * 1.75;

    for (int i = 0; i < numSamples; ++i)
    {
        smoothedRatio += ratioDelta;

        // 1. Write incoming sample to circular delay buffer
        leftBuffer[writePos] = leftData[i];
        if (rightData != nullptr)
            rightBuffer[writePos] = rightData[i];

        // 2. Advance primary read position by the pitch ratio
        primaryReadPos += static_cast<double>(smoothedRatio);
        while (primaryReadPos >= static_cast<double>(kBufferSize))
            primaryReadPos -= static_cast<double>(kBufferSize);

        // Calculate current delay of primary pointer behind write pointer
        double currentDelay = static_cast<double>(writePos) - primaryReadPos;
        while (currentDelay < 0.0)
            currentDelay += static_cast<double>(kBufferSize);
        while (currentDelay >= static_cast<double>(kBufferSize))
            currentDelay -= static_cast<double>(kBufferSize);

        // 3. Unvoiced / silence gentle drift back to nominal delay center
        if (!isCrossfading && (!isVoicedFrame || std::abs(smoothedRatio - 1.0f) < 0.0001f))
        {
            const double delayDiff = nominalDelay - currentDelay;
            if (std::abs(delayDiff) > 2.0)
            {
                // Nudge by 0.0005 samples per audio sample (completely inaudible)
                const double nudge = (delayDiff > 0.0) ? -0.0005 : 0.0005;
                primaryReadPos += nudge;
            }
        }

        // 4. Check for boundary wrap and initiate seamless in-phase splice crossfade
        if (!isCrossfading && std::abs(smoothedRatio - 1.0f) > 0.0001f)
        {
            if (currentDelay < minDelay)
            {
                // Reading too fast (pitching UP) -> Jump back by integer periods (k * T0)
                const double delta = nominalDelay - currentDelay;
                const int periods = std::max(1, static_cast<int>(std::round(delta / static_cast<double>(T0))));
                const double safeJump = static_cast<double>(periods) * static_cast<double>(T0);

                double targetPos = primaryReadPos - safeJump;
                while (targetPos < 0.0)
                    targetPos += static_cast<double>(kBufferSize);

                // Phase Micro-Locking: search +/- 4 samples to align with exact acoustic wave crest
                double bestOffset = 0.0;
                float maxCorr = -1e9f;
                const float refSample = readCubic(leftBuffer, primaryReadPos);

                for (int d = -4; d <= 4; ++d)
                {
                    const double testPos = targetPos + static_cast<double>(d);
                    const float testSample = readCubic(leftBuffer, testPos);
                    const float corr = refSample * testSample;
                    if (corr > maxCorr)
                    {
                        maxCorr = corr;
                        bestOffset = static_cast<double>(d);
                    }
                }

                secondaryReadPos = targetPos + bestOffset;
                while (secondaryReadPos < 0.0)
                    secondaryReadPos += static_cast<double>(kBufferSize);
                while (secondaryReadPos >= static_cast<double>(kBufferSize))
                    secondaryReadPos -= static_cast<double>(kBufferSize);

                isCrossfading = true;
                xfadeProgress = 0.0f;
                // Crossfade over 1 pitch period (clamped 32 to 256 samples)
                const float xfadeSamples = juce::jlimit(32.0f, 256.0f, T0);
                xfadeStep = 1.0f / xfadeSamples;
            }
            else if (currentDelay > maxDelay)
            {
                // Reading too slow (pitching DOWN) -> Jump forward by integer periods (k * T0)
                const double delta = currentDelay - nominalDelay;
                const int periods = std::max(1, static_cast<int>(std::round(delta / static_cast<double>(T0))));
                const double safeJump = static_cast<double>(periods) * static_cast<double>(T0);

                double targetPos = primaryReadPos + safeJump;
                while (targetPos >= static_cast<double>(kBufferSize))
                    targetPos -= static_cast<double>(kBufferSize);

                // Phase Micro-Locking: search +/- 4 samples
                double bestOffset = 0.0;
                float maxCorr = -1e9f;
                const float refSample = readCubic(leftBuffer, primaryReadPos);

                for (int d = -4; d <= 4; ++d)
                {
                    const double testPos = targetPos + static_cast<double>(d);
                    const float testSample = readCubic(leftBuffer, testPos);
                    const float corr = refSample * testSample;
                    if (corr > maxCorr)
                    {
                        maxCorr = corr;
                        bestOffset = static_cast<double>(d);
                    }
                }

                secondaryReadPos = targetPos + bestOffset;
                while (secondaryReadPos < 0.0)
                    secondaryReadPos += static_cast<double>(kBufferSize);
                while (secondaryReadPos >= static_cast<double>(kBufferSize))
                    secondaryReadPos -= static_cast<double>(kBufferSize);

                isCrossfading = true;
                xfadeProgress = 0.0f;
                const float xfadeSamples = juce::jlimit(32.0f, 256.0f, T0);
                xfadeStep = 1.0f / xfadeSamples;
            }
        }

        // 4. Synthesize Output Sample
        if (!isCrossfading)
        {
            // SINGLE POINTER ACTIVE: 100% Pure Vocal, ZERO Flanging/Comb-filtering!
            leftData[i] = readCubic(leftBuffer, primaryReadPos);
            if (rightData != nullptr)
                rightData[i] = readCubic(rightBuffer, primaryReadPos);
        }
        else
        {
            // Brief in-phase splice crossfade
            secondaryReadPos += static_cast<double>(smoothedRatio);
            while (secondaryReadPos >= static_cast<double>(kBufferSize))
                secondaryReadPos -= static_cast<double>(kBufferSize);

            xfadeProgress += xfadeStep;

            // Equal-power / Hann crossfade across identical pitch phase
            const float fadeNorm = juce::jlimit(0.0f, 1.0f, xfadeProgress);
            const float wOld = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * fadeNorm));
            const float wNew = 1.0f - wOld;

            const float s1L = readCubic(leftBuffer, primaryReadPos);
            const float s2L = readCubic(leftBuffer, secondaryReadPos);
            leftData[i] = (s1L * wOld) + (s2L * wNew);

            if (rightData != nullptr)
            {
                const float s1R = readCubic(rightBuffer, primaryReadPos);
                const float s2R = readCubic(rightBuffer, secondaryReadPos);
                rightData[i] = (s1R * wOld) + (s2R * wNew);
            }

            if (xfadeProgress >= 1.0f)
            {
                // Crossfade finished: secondary pointer becomes the sole active pointer
                isCrossfading = false;
                primaryReadPos = secondaryReadPos;
            }
        }

        writePos = (writePos + 1) % kBufferSize;
    }
}

} // namespace toytune
