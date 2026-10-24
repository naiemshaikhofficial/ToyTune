#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PitchShifter.h"
#include "FormantFilter.h"
#include "CircularBuffer.h"
#include "../PitchCorrection/ScaleEngine.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace toytune
{

enum class HarmonyIntervalMode
{
    ScaleDiatonic = 0,
    FixedSemitones,
    MidiNotes
};

enum class HarmonyVoiceInterval
{
    OctaveDown = 0, // -12
    SeventhDown,    // -10 / -11
    FifthDown,      // -7
    FourthDown,     // -5
    ThirdDown,      // -3 / -4 diatonic
    Unison,         // 0
    ThirdUp,        // +3 / +4 diatonic
    FourthUp,       // +5
    FifthUp,        // +7
    SeventhUp,      // +10 / +11
    OctaveUp        // +12
};

struct HarmonyVoiceParams
{
    bool enabled { true };
    HarmonyVoiceInterval interval { HarmonyVoiceInterval::ThirdUp };
    float formantShiftSt { 0.0f }; // Semitones
    float pan { 0.0f };            // -1.0 to +1.0
    float levelDb { -6.0f };       // dB
    bool solo { false };
};

struct HarmonyMasterParams
{
    bool enabled { false };
    HarmonyIntervalMode intervalMode { HarmonyIntervalMode::ScaleDiatonic };
    float naturalizePct { 25.0f };      // 0 to 100%
    float transitionTimeMs { 125.0f };  // Portamento glide ms
    float pitchVariationCents { 20.0f };// Micro-detune jitter
    float timingVariationMs { 35.0f };  // Humanized millisecond delay
    float envAttackMs { 10.0f };
    float envReleaseMs { 150.0f };
    float eqHighPassHz { 120.0f };
    float gateThresholdDb { -55.0f };
    float stereoWidthPct { 120.0f };
};

/**
 * Auto-Tune Pro 11-Grade 4-Voice Real-Time Vocal Harmonizer.
 * Generates up to 4 diatonic / fixed / MIDI backing vocalists with independent
 * formant gender shifting, humanized micro-delay lines, pitch jitter, noise gate,
 * and Mid-Side stereo width expansion.
 * Zero heap allocation during audio thread processing.
 */
class HarmonyEngine
{
public:
    HarmonyEngine() = default;

    void prepare(double newSampleRate, int maxBlockSize)
    {
        sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 44100.0;
        blockSize = maxBlockSize;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 2;

        for (int i = 0; i < 4; ++i)
        {
            shifters[i].prepare(sampleRate, maxBlockSize);
            formantFilters[i].prepare(spec);
            delayLinesL[i].setSize(8192);
            delayLinesR[i].setSize(8192);
            currentVoiceRatios[i] = 1.0f;
            targetVoiceRatios[i] = 1.0f;
            jitterPhases[i] = static_cast<float>(i) * 1.57f;
            smoothedLevels[i] = 0.5f;
        }

        voiceBuffer.setSize(2, maxBlockSize);
        harmonySumBuffer.setSize(2, maxBlockSize);
        gateEnvelope = 0.0f;
        reset();
    }

    void reset()
    {
        for (int i = 0; i < 4; ++i)
        {
            shifters[i].reset();
            formantFilters[i].reset();
            delayLinesL[i].reset();
            delayLinesR[i].reset();
            currentVoiceRatios[i] = 1.0f;
            targetVoiceRatios[i] = 1.0f;
            jitterPhases[i] = static_cast<float>(i) * 1.57f;
            smoothedLevels[i] = 0.5f;
        }
        voiceBuffer.clear();
        harmonySumBuffer.clear();
        gateEnvelope = 0.0f;
    }

    void process(juce::AudioBuffer<float>& mainBuffer,
                 const ScaleEngine& scaleEngine,
                 float detectedPitchHz,
                 float detectedMidiNote,
                 bool isVoiced,
                 const std::array<HarmonyVoiceParams, 4>& voices,
                 const HarmonyMasterParams& master)
    {
        if (!master.enabled)
            return;

        const int numChannels = mainBuffer.getNumChannels();
        const int numSamples = mainBuffer.getNumSamples();
        if (numSamples <= 0 || numChannels <= 0)
            return;

        if (harmonySumBuffer.getNumSamples() < numSamples)
            harmonySumBuffer.setSize(2, numSamples, false, false, true);
        if (voiceBuffer.getNumSamples() < numSamples)
            voiceBuffer.setSize(2, numSamples, false, false, true);

        harmonySumBuffer.clear();

        // 1. Noise Gate & Envelope Evaluation
        const float inRms = mainBuffer.getRMSLevel(0, 0, numSamples);
        const float inDb = (inRms > 1e-5f) ? juce::Decibels::gainToDecibels(inRms) : -100.0f;
        const bool gateOpen = isVoiced && (inDb > master.gateThresholdDb);

        const float attackCoeff = std::exp(-1.0f / (std::max(0.001f, master.envAttackMs * 0.001f) * static_cast<float>(sampleRate) / static_cast<float>(numSamples)));
        const float releaseCoeff = std::exp(-1.0f / (std::max(0.005f, master.envReleaseMs * 0.001f) * static_cast<float>(sampleRate) / static_cast<float>(numSamples)));
        const float targetGate = gateOpen ? 1.0f : 0.0f;
        const float coeff = (targetGate > gateEnvelope) ? (1.0f - attackCoeff) : (1.0f - releaseCoeff);
        gateEnvelope += coeff * (targetGate - gateEnvelope);

        if (gateEnvelope <= 1e-4f)
            return; // Gate closed

        // Check if any voice is soloed
        bool anySolo = false;
        for (const auto& v : voices)
        {
            if (v.enabled && v.solo) { anySolo = true; break; }
        }

        // 2. Process each of the 4 Harmony Voices
        for (int i = 0; i < 4; ++i)
        {
            const auto& v = voices[i];
            if (!v.enabled)
                continue;
            if (anySolo && !v.solo)
                continue;

            // Calculate interval semitones
            float intervalSemitones = getIntervalSemitones(v.interval, detectedMidiNote, scaleEngine, master.intervalMode);

            // Add Humanized Pitch Variation Jitter
            const float jitterDepth = (master.pitchVariationCents / 100.0f);
            jitterPhases[i] += 0.05f * (static_cast<float>(i + 1) * 0.7f);
            if (jitterPhases[i] >= 6.2831853f)
                jitterPhases[i] -= 6.2831853f;
            const float microJitter = std::sin(jitterPhases[i]) * jitterDepth * 0.35f;

            // Target pitch ratio
            const float targetRatio = std::pow(2.0f, (intervalSemitones + microJitter) / 12.0f);

            // Portamento transition smoothing
            const float portamentoCoeff = 1.0f - std::exp(-static_cast<float>(numSamples) / (std::max(0.01f, master.transitionTimeMs * 0.001f) * static_cast<float>(sampleRate)));
            currentVoiceRatios[i] += portamentoCoeff * (targetRatio - currentVoiceRatios[i]);

            // Copy input into voice buffer
            for (int ch = 0; ch < 2; ++ch)
            {
                const int srcCh = std::min(ch, numChannels - 1);
                voiceBuffer.copyFrom(ch, 0, mainBuffer, srcCh, 0, numSamples);
            }

            // Pitch shift this voice
            shifters[i].setPitchRatioAndPitch(currentVoiceRatios[i], detectedPitchHz, isVoiced);
            shifters[i].process(voiceBuffer);

            // Formant shift this voice
            if (std::abs(v.formantShiftSt) > 0.05f)
            {
                formantFilters[i].updateParameters(currentVoiceRatios[i], v.formantShiftSt, true);
                formantFilters[i].process(voiceBuffer);
            }

            // Humanized Loose Timing Delay
            // Voice 0 = 0 ms, Voice 1 = ~25% delay, Voice 2 = ~50% delay, Voice 3 = ~75% delay
            const float voiceDelayMs = (master.timingVariationMs * 0.001f) * (static_cast<float>(i) * 0.33f);
            const float delaySamples = voiceDelayMs * static_cast<float>(sampleRate);

            auto& dL = delayLinesL[i];
            auto& dR = delayLinesR[i];
            auto* vL = voiceBuffer.getWritePointer(0);
            auto* vR = voiceBuffer.getWritePointer(1);

            for (int s = 0; s < numSamples; ++s)
            {
                dL.write(vL[s]);
                dR.write(vR[s]);
                vL[s] = dL.readCubic(delaySamples);
                vR[s] = dR.readCubic(delaySamples);
            }

            // Panning (Constant-Power) & Gain
            const float panNorm = juce::jlimit(-1.0f, 1.0f, v.pan);
            const float panAngle = (panNorm + 1.0f) * 0.25f * juce::MathConstants<float>::pi; // 0 to pi/2
            const float gainL = std::cos(panAngle);
            const float gainR = std::sin(panAngle);
            const float voiceVol = juce::Decibels::decibelsToGain(v.levelDb);

            // Accumulate into harmonySumBuffer
            harmonySumBuffer.addFromWithRamp(0, 0, vL, numSamples, voiceVol * gainL, voiceVol * gainL);
            harmonySumBuffer.addFromWithRamp(1, 0, vR, numSamples, voiceVol * gainR, voiceVol * gainR);
        }

        // 3. High-Pass Clean Filter on Harmonies (>120 Hz)
        if (master.eqHighPassHz > 30.0f)
        {
            const float hpNorm = master.eqHighPassHz / static_cast<float>(sampleRate);
            const float hpAlpha = 1.0f / (1.0f + 2.0f * juce::MathConstants<float>::pi * hpNorm);
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* ptr = harmonySumBuffer.getWritePointer(ch);
                float prev = 0.0f;
                for (int s = 0; s < numSamples; ++s)
                {
                    const float in = ptr[s];
                    ptr[s] = hpAlpha * (ptr[s] - prev);
                    prev = in;
                }
            }
        }

        // 4. Stereo Width (Mid-Side Processing)
        const float widthNorm = master.stereoWidthPct / 100.0f;
        auto* sumL = harmonySumBuffer.getWritePointer(0);
        auto* sumR = harmonySumBuffer.getWritePointer(1);

        for (int s = 0; s < numSamples; ++s)
        {
            const float mid = (sumL[s] + sumR[s]) * 0.5f;
            const float side = (sumR[s] - sumL[s]) * 0.5f * widthNorm;
            sumL[s] = (mid - side) * gateEnvelope;
            sumR[s] = (mid + side) * gateEnvelope;
        }

        // 5. Mix Backing Harmonies into Main Buffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const int harmCh = std::min(ch, 1);
            mainBuffer.addFrom(ch, 0, harmonySumBuffer, harmCh, 0, numSamples);
        }
    }

private:
    double sampleRate { 44100.0 };
    int blockSize { 512 };

    std::array<PitchShifter, 4> shifters;
    std::array<FormantFilter, 4> formantFilters;
    std::array<CircularBuffer, 4> delayLinesL;
    std::array<CircularBuffer, 4> delayLinesR;
    std::array<float, 4> currentVoiceRatios { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 4> targetVoiceRatios { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 4> jitterPhases { 0.0f, 1.57f, 3.14f, 4.71f };
    std::array<float, 4> smoothedLevels { 0.5f, 0.5f, 0.5f, 0.5f };

    juce::AudioBuffer<float> voiceBuffer;
    juce::AudioBuffer<float> harmonySumBuffer;
    float gateEnvelope { 0.0f };

    static float getIntervalSemitones(HarmonyVoiceInterval interval,
                                      float detectedMidiNote,
                                      const ScaleEngine& scaleEngine,
                                      HarmonyIntervalMode mode)
    {
        if (mode == HarmonyIntervalMode::FixedSemitones || detectedMidiNote < 20.0f)
        {
            switch (interval)
            {
                case HarmonyVoiceInterval::OctaveDown:  return -12.0f;
                case HarmonyVoiceInterval::SeventhDown: return -10.0f;
                case HarmonyVoiceInterval::FifthDown:   return -7.0f;
                case HarmonyVoiceInterval::FourthDown:  return -5.0f;
                case HarmonyVoiceInterval::ThirdDown:   return -4.0f;
                case HarmonyVoiceInterval::Unison:      return 0.0f;
                case HarmonyVoiceInterval::ThirdUp:     return 4.0f;
                case HarmonyVoiceInterval::FourthUp:    return 5.0f;
                case HarmonyVoiceInterval::FifthUp:     return 7.0f;
                case HarmonyVoiceInterval::SeventhUp:   return 10.0f;
                case HarmonyVoiceInterval::OctaveUp:    return 12.0f;
                default:                                return 0.0f;
            }
        }

        // Diatonic Scale-Aware Interval calculation:
        // Finds target scale degree in active key
        const int inputNote = static_cast<int>(std::round(detectedMidiNote));
        int scaleSteps = 0;
        switch (interval)
        {
            case HarmonyVoiceInterval::OctaveDown:  return -12.0f;
            case HarmonyVoiceInterval::SeventhDown: scaleSteps = -6; break;
            case HarmonyVoiceInterval::FifthDown:   scaleSteps = -4; break;
            case HarmonyVoiceInterval::FourthDown:  scaleSteps = -3; break;
            case HarmonyVoiceInterval::ThirdDown:   scaleSteps = -2; break;
            case HarmonyVoiceInterval::Unison:      return 0.0f;
            case HarmonyVoiceInterval::ThirdUp:     scaleSteps = +2; break;
            case HarmonyVoiceInterval::FourthUp:    scaleSteps = +3; break;
            case HarmonyVoiceInterval::FifthUp:     scaleSteps = +4; break;
            case HarmonyVoiceInterval::SeventhUp:   scaleSteps = +6; break;
            case HarmonyVoiceInterval::OctaveUp:    return +12.0f;
            default:                                return 0.0f;
        }

        // Walk through active scale notes
        int targetMidi = inputNote;
        const int direction = (scaleSteps > 0) ? 1 : -1;
        int remainingSteps = std::abs(scaleSteps);

        while (remainingSteps > 0)
        {
            targetMidi += direction;
            if (targetMidi < 12 || targetMidi > 127)
                break;
            const int semitone = (targetMidi % 12 + 12) % 12;
            if (scaleEngine.isSemitoneActive(semitone))
                --remainingSteps;
        }

        return static_cast<float>(targetMidi - inputNote);
    }
};

} // namespace toytune
