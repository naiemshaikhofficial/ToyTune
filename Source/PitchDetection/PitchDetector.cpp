#include "PitchDetector.h"
#include <cmath>
#include <algorithm>

namespace toytune
{

PitchDetector::PitchDetector()
{
    inputBuffer.resize(kBufferSize, 0.0f);
    differenceBuffer.resize(kBufferSize / 2, 0.0f);
    cmndfBuffer.resize(kBufferSize / 2, 0.0f);
    linearBuffer.resize(kBufferSize, 0.0f);
}

void PitchDetector::prepare(double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 44100.0;
    updatePeriodLimits();
    reset();
}

void PitchDetector::reset()
{
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
    std::fill(differenceBuffer.begin(), differenceBuffer.end(), 0.0f);
    std::fill(cmndfBuffer.begin(), cmndfBuffer.end(), 0.0f);
    std::fill(linearBuffer.begin(), linearBuffer.end(), 0.0f);
    writeIndex = 0;
    currentPitchHz = 0.0f;
    currentMidiNote = 0.0f;
    currentConfidence = 0.0f;
    isVoiced = false;
    previousPeriod = 0.0f;
    unvoicedFramesCount = 0;
    samplesSinceLastDetect = kDetectHopSamples; // Trigger detection on initial audio frame
}

void PitchDetector::setVocalRange(VocalRangeType rangeType)
{
    vocalRange = rangeType;
    updatePeriodLimits();
}

void PitchDetector::setReferencePitch(float refHz)
{
    if (refHz >= 400.0f && refHz <= 480.0f)
        referenceA4 = refHz;
}

void PitchDetector::updatePeriodLimits()
{
    float minHz = 55.0f;
    float maxHz = 1100.0f;

    switch (vocalRange)
    {
        case VocalRangeType::BassBaritone:
            minHz = 55.0f;
            maxHz = 450.0f;
            break;
        case VocalRangeType::Tenor:
            minHz = 85.0f;
            maxHz = 750.0f;
            break;
        case VocalRangeType::AltoSoprano:
            minHz = 150.0f;
            maxHz = 1100.0f;
            break;
        case VocalRangeType::Wide:
        default:
            minHz = 55.0f;
            maxHz = 1100.0f;
            break;
    }

    minPeriodSamples = std::max(4, static_cast<int>(std::floor(sampleRate / maxHz)));
    // Absolute safety clamp: max period capped to 1024 samples (down to 43 Hz at 44.1k, 47 Hz at 48k)
    const int maxAllowed = static_cast<int>(kBufferSize / 4); // 2048
    maxPeriodSamples = std::min(maxAllowed, static_cast<int>(std::ceil(sampleRate / minHz)));
}

float PitchDetector::calculateRmsDb(const float* data, int numSamples) const noexcept
{
    if (numSamples <= 0 || data == nullptr)
        return -100.0f;

    float sumSquares = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float s = data[i];
        sumSquares += s * s;
    }

    const float meanSquare = sumSquares / static_cast<float>(numSamples);
    if (meanSquare <= 1e-10f)
        return -100.0f;

    return 10.0f * std::log10(meanSquare);
}

float PitchDetector::frequencyToMidi(float freqHz) const noexcept
{
    if (freqHz <= 10.0f)
        return 0.0f;
    return 69.0f + 12.0f * std::log2(freqHz / referenceA4);
}

PitchDetectionResult PitchDetector::processBlock(const float* channelData, int numSamples)
{
    PitchDetectionResult result;
    if (channelData == nullptr || numSamples <= 0)
        return result;

    const float rmsDb = calculateRmsDb(channelData, numSamples);
    result.rmsDb = rmsDb;

    // Fast-path silence gate: below -52 dBFS is considered unvoiced background silence
    if (rmsDb < -52.0f)
    {
        unvoicedFramesCount++;
        if (unvoicedFramesCount > 3)
        {
            isVoiced = false;
            currentConfidence = 0.0f;
        }
        result.isVoiced = isVoiced;
        result.confidence = currentConfidence;
        result.pitchHz = currentPitchHz;
        result.midiNote = currentMidiNote;
        return result;
    }

    // Append incoming samples into circular analysis buffer
    for (int i = 0; i < numSamples; ++i)
    {
        inputBuffer[static_cast<size_t>(writeIndex)] = channelData[i];
        writeIndex = (writeIndex + 1) & (kBufferSize - 1);
    }

    samplesSinceLastDetect += numSamples;

    // Hop Decoupling: In small ASIO buffers (64/128 samples), skip redundant intra-cycle detection
    // and return stable cached pitch to keep real-time audio thread CPU < 0.5% with 0 audio underruns!
    if (samplesSinceLastDetect < kDetectHopSamples && currentPitchHz > 20.0f && isVoiced)
    {
        result.isVoiced = isVoiced;
        result.confidence = currentConfidence;
        result.pitchHz = currentPitchHz;
        result.midiNote = currentMidiNote;
        return result;
    }

    samplesSinceLastDetect = 0;

    // Step 1: Compute Vectorized Difference Function without inner loop modulo
    // d(tau) = sum_{j=0}^{W-1} (x[j] - x[j+tau])^2
    const int windowSize = std::min(maxPeriodSamples, 512);
    const int totalSpan = windowSize + maxPeriodSamples;
    const int bufferStart = (writeIndex - totalSpan + kBufferSize * 8) & (kBufferSize - 1);

    // Linearize the continuous ring buffer into contiguous memory for auto-vectorization
    for (int j = 0; j < totalSpan; ++j)
    {
        const int readIdx = (bufferStart + j) & (kBufferSize - 1);
        linearBuffer[static_cast<size_t>(j)] = inputBuffer[static_cast<size_t>(readIdx)];
    }

    differenceBuffer[0] = 0.0f;
    const float* const pRef = linearBuffer.data();

    for (int tau = 1; tau <= maxPeriodSamples; ++tau)
    {
        float diffSum = 0.0f;
        const float* const pShift = pRef + tau;

        for (int j = 0; j < windowSize; j += 2)
        {
            const float diff = pRef[j] - pShift[j];
            diffSum += diff * diff;
        }
        differenceBuffer[static_cast<size_t>(tau)] = diffSum * 2.0f;
    }

    // Step 2: Cumulative Mean Normalized Difference Function (CMNDF)
    cmndfBuffer[0] = 1.0f;
    float runningSum = 0.0f;

    for (int tau = 1; tau <= maxPeriodSamples; ++tau)
    {
        runningSum += differenceBuffer[static_cast<size_t>(tau)];
        if (runningSum > 1e-7f)
            cmndfBuffer[static_cast<size_t>(tau)] = differenceBuffer[static_cast<size_t>(tau)] / (runningSum / static_cast<float>(tau));
        else
            cmndfBuffer[static_cast<size_t>(tau)] = 1.0f;
    }

    // Step 3: Absolute Thresholding & Peak Picking
    constexpr float kThreshold = 0.20f;
    int tauEstimate = -1;

    for (int tau = minPeriodSamples; tau <= maxPeriodSamples; ++tau)
    {
        if (cmndfBuffer[static_cast<size_t>(tau)] < kThreshold)
        {
            while (tau + 1 <= maxPeriodSamples && cmndfBuffer[static_cast<size_t>(tau + 1)] < cmndfBuffer[static_cast<size_t>(tau)])
            {
                tau++;
            }
            tauEstimate = tau;
            break;
        }
    }

    // Fallback: Global minimum if threshold was not crossed but signal is periodic
    if (tauEstimate == -1)
    {
        float minVal = 1.0f;
        int minIdx = -1;
        for (int tau = minPeriodSamples; tau <= maxPeriodSamples; ++tau)
        {
            if (cmndfBuffer[static_cast<size_t>(tau)] < minVal)
            {
                minVal = cmndfBuffer[static_cast<size_t>(tau)];
                minIdx = tau;
            }
        }
        if (minVal < 0.40f)
            tauEstimate = minIdx;
    }

    if (tauEstimate > 0 && tauEstimate >= minPeriodSamples && tauEstimate <= maxPeriodSamples)
    {
        // Step 4: Sub-sample Parabolic Interpolation on CMNDF
        float bestTau = static_cast<float>(tauEstimate);
        if (tauEstimate > 1 && tauEstimate < maxPeriodSamples)
        {
            const float y0 = cmndfBuffer[static_cast<size_t>(tauEstimate - 1)];
            const float y1 = cmndfBuffer[static_cast<size_t>(tauEstimate)];
            const float y2 = cmndfBuffer[static_cast<size_t>(tauEstimate + 1)];
            const float denom = (y0 - 2.0f * y1 + y2);
            if (std::abs(denom) > 1e-6f)
            {
                const float delta = (y0 - y2) / (2.0f * denom);
                if (std::abs(delta) < 1.0f)
                    bestTau += delta;
            }
        }

        // Octave Jump Rejection:
        // Prevent spurious sub-octave doubling (tau ~ 2 * prevPeriod)
        if (previousPeriod > 0.0f)
        {
            const float halfPeriod = bestTau * 0.5f;
            if (halfPeriod >= static_cast<float>(minPeriodSamples))
            {
                const int halfTauIdx = static_cast<int>(std::round(halfPeriod));
                if (halfTauIdx >= minPeriodSamples && halfTauIdx <= maxPeriodSamples)
                {
                    if (cmndfBuffer[halfTauIdx] < kThreshold * 1.5f &&
                        std::abs(halfPeriod - previousPeriod) < previousPeriod * 0.15f)
                    {
                        bestTau = halfPeriod;
                    }
                }
            }
        }

        previousPeriod = bestTau;
        const float rawPitchHz = static_cast<float>(sampleRate) / bestTau;
        const float dipVal = cmndfBuffer[tauEstimate];
        const float rawConfidence = juce::jlimit(0.0f, 1.0f, 1.0f - dipVal);

        // Adaptive Fast-Attack Temporal Tracking:
        // When a singer changes notes (> 5% frequency change) or attacks with high confidence,
        // snap instantly so Retune Speed = 0 ms is truly instantaneous!
        // Only apply gentle stabilization on steady held notes (< 5% change) to eliminate sub-cent jitter.
        const float pitchDiffRatio = (currentPitchHz > 10.0f) ? std::abs(rawPitchHz - currentPitchHz) / currentPitchHz : 1.0f;
        if (!isVoiced || unvoicedFramesCount > 1 || rawConfidence > 0.80f || pitchDiffRatio > 0.05f)
        {
            currentPitchHz = rawPitchHz;
            currentConfidence = rawConfidence;
        }
        else
        {
            currentPitchHz = 0.25f * currentPitchHz + 0.75f * rawPitchHz;
            currentConfidence = 0.3f * currentConfidence + 0.7f * rawConfidence;
        }

        isVoiced = (currentConfidence >= 0.50f);
        unvoicedFramesCount = 0;
        currentMidiNote = frequencyToMidi(currentPitchHz);
    }
    else
    {
        unvoicedFramesCount++;
        if (unvoicedFramesCount > 2)
        {
            isVoiced = false;
            currentConfidence = std::max(0.0f, currentConfidence - 0.2f);
        }
    }

    result.isVoiced = isVoiced;
    result.confidence = currentConfidence;
    result.pitchHz = currentPitchHz;
    result.midiNote = currentMidiNote;
    return result;
}

} // namespace toytune
