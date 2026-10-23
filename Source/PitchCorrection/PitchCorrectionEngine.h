#pragma once

#include "../PitchDetection/PitchDetector.h"
#include "ScaleEngine.h"
#include "TargetNoteSelector.h"
#include "../DSP/CorrectionSmoother.h"
#include "../DSP/PitchShifter.h"
#include "../DSP/FormantFilter.h"
#include "../DSP/CircularBuffer.h"
#include "../DSP/AudioTelemetry.h"
#include <juce_dsp/juce_dsp.h>

namespace toytune
{

class PitchCorrectionEngine
{
public:
    PitchCorrectionEngine() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        detector.prepare(sampleRate, maxBlockSize);
        shifter.prepare(sampleRate, maxBlockSize);
        smoother.prepare(sampleRate);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 2;
        formantFilter.prepare(spec);

        dryBuffer.setSize(2, maxBlockSize);
        monoSumBuffer.setSize(1, maxBlockSize);
        dryDelayLineL.setSize(8192);
        dryDelayLineR.setSize(8192);
        reset();
    }

    void reset()
    {
        detector.reset();
        shifter.reset();
        smoother.reset();
        targetSelector.reset();
        formantFilter.reset();
        dryBuffer.clear();
        monoSumBuffer.clear();
        dryDelayLineL.reset();
        dryDelayLineR.reset();
    }

    void processMidi(const juce::MidiBuffer& midiMessages)
    {
        targetSelector.processMidi(midiMessages);
    }

    void setLowLatencyMode(bool lowLatency)
    {
        shifter.setLowLatencyMode(lowLatency);
    }

    int getLatencySamples() const noexcept
    {
        return shifter.getLatencySamples();
    }

    void process(juce::AudioBuffer<float>& buffer,
                 const ScaleEngine& scaleEngine,
                 AudioTelemetry& telemetry,
                 float inputGainDb,
                 float outputGainDb,
                 float retuneSpeedMs,
                 float strengthPct,
                 float toleranceCents,
                 float humanizePct,
                 float flexTunePct,
                 float vibratoPreservePct,
                 TuningModeType tuningMode,
                 VocalRangeType vocalRange,
                 float refPitchHz,
                 bool formantPreserve,
                 float formantShiftSt,
                 float dryWetMixPct,
                 bool isBypassed)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        if (numSamples <= 0 || numChannels <= 0)
            return;

        // Apply Input Gain
        const float inGainLinear = juce::Decibels::decibelsToGain(inputGainDb);
        buffer.applyGain(inGainLinear);

        // Calculate Input RMS
        const float inRms = buffer.getRMSLevel(0, 0, numSamples);
        const float inRmsDb = (inRms > 1e-5f) ? juce::Decibels::gainToDecibels(inRms) : -100.0f;
        telemetry.inputRmsDb.store(inRmsDb, std::memory_order_relaxed);

        if (isBypassed)
        {
            telemetry.outputRmsDb.store(inRmsDb, std::memory_order_relaxed);
            return;
        }

        // Copy dry signal for mix
        if (dryBuffer.getNumSamples() < numSamples)
            dryBuffer.setSize(numChannels, numSamples, false, false, true);

        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        // 1. Prepare mono sum for pitch detection
        if (monoSumBuffer.getNumSamples() < numSamples)
            monoSumBuffer.setSize(1, numSamples, false, false, true);

        monoSumBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        if (numChannels > 1)
        {
            monoSumBuffer.addFrom(0, 0, buffer, 1, 0, numSamples);
            monoSumBuffer.applyGain(0.5f);
        }

        // 2. Fundamental Frequency Detection
        detector.setVocalRange(vocalRange);
        detector.setReferencePitch(refPitchHz);
        const auto detectResult = detector.processBlock(monoSumBuffer.getReadPointer(0), numSamples);

        telemetry.detectedPitchHz.store(detectResult.pitchHz, std::memory_order_relaxed);
        telemetry.detectedMidiNote.store(detectResult.midiNote, std::memory_order_relaxed);
        telemetry.pitchConfidence.store(detectResult.confidence, std::memory_order_relaxed);
        telemetry.isVoiced.store(detectResult.isVoiced, std::memory_order_relaxed);

        // 3. Musical Target Selection
        float pitchRatio = 1.0f;
        float centsDev = 0.0f;

        if (detectResult.isVoiced && detectResult.pitchHz > 20.0f)
        {
            const auto target = targetSelector.resolveTarget(scaleEngine, detectResult.midiNote);
            centsDev = target.centsDeviation;

            telemetry.targetMidiNote.store(target.targetMidiNote, std::memory_order_relaxed);
            telemetry.targetPitchHz.store(target.targetFreqHz, std::memory_order_relaxed);
            telemetry.centsDeviation.store(centsDev, std::memory_order_relaxed);

            // 4. Correction Smoothing
            pitchRatio = smoother.processBlock(centsDev,
                                               retuneSpeedMs,
                                               strengthPct,
                                               toleranceCents,
                                               humanizePct,
                                               flexTunePct,
                                               vibratoPreservePct,
                                               tuningMode,
                                               numSamples);

            telemetry.pushHistoryPoint(detectResult.pitchHz, target.targetFreqHz);
        }
        else
        {
            // Unvoiced / silence: natural unity pass
            pitchRatio = 1.0f;
            telemetry.centsDeviation.store(0.0f, std::memory_order_relaxed);
            telemetry.pushHistoryPoint(0.0f, 0.0f);
        }

        telemetry.currentRetuneRatio.store(pitchRatio, std::memory_order_relaxed);

        // 5. Real-Time Pitch-Synchronous Intonation Shifter
        shifter.setPitchRatioAndPitch(pitchRatio, detectResult.pitchHz, detectResult.isVoiced);
        shifter.process(buffer);

        // 6. Formant Filter
        formantFilter.updateParameters(pitchRatio, formantShiftSt, formantPreserve);
        formantFilter.process(buffer);

        // 7. Dry/Wet Mix with Phase-Aligned Delay Compensation
        if (dryWetMixPct < 99.5f)
        {
            const float wetGain = dryWetMixPct / 100.0f;
            const float dryGain = 1.0f - wetGain;
            const float latency = static_cast<float>(shifter.getLatencySamples());

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* outPtr = buffer.getWritePointer(ch);
                const auto* dryPtr = dryBuffer.getReadPointer(ch);
                auto& delayLine = (ch == 0) ? dryDelayLineL : dryDelayLineR;
                for (int i = 0; i < numSamples; ++i)
                {
                    delayLine.write(dryPtr[i]);
                    const float delayedDry = delayLine.readCubic(latency);
                    outPtr[i] = (delayedDry * dryGain) + (outPtr[i] * wetGain);
                }
            }
        }
        else
        {
            // Pure wet fast path (Mix = 100%)
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const auto* dryPtr = dryBuffer.getReadPointer(ch);
                auto& delayLine = (ch == 0) ? dryDelayLineL : dryDelayLineR;
                for (int i = 0; i < numSamples; ++i)
                    delayLine.write(dryPtr[i]);
            }
        }

        // 8. Output Gain
        const float outGainLinear = juce::Decibels::decibelsToGain(outputGainDb);
        buffer.applyGain(outGainLinear);

        // Output RMS for meter
        const float outRms = buffer.getRMSLevel(0, 0, numSamples);
        const float outRmsDb = (outRms > 1e-5f) ? juce::Decibels::gainToDecibels(outRms) : -100.0f;
        telemetry.outputRmsDb.store(outRmsDb, std::memory_order_relaxed);
    }

private:
    PitchDetector detector;
    PitchShifter shifter;
    CorrectionSmoother smoother;
    TargetNoteSelector targetSelector;
    FormantFilter formantFilter;

    CircularBuffer dryDelayLineL;
    CircularBuffer dryDelayLineR;

    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> monoSumBuffer;
};

} // namespace toytune
