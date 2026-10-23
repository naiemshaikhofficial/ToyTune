#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>
#include "../Parameters/ToyTuneParameters.h"

namespace toytune
{

class CorrectionSmoother
{
public:
    CorrectionSmoother() = default;

    void prepare(double newSampleRate)
    {
        sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        smoothedPitchShiftCents = 0.0f;
        previousTargetCents = 0.0f;
        vibratoBuffer = 0.0f;
    }

    /**
     * Compute the smoothed pitch shift ratio to apply to the PitchShifter.
     * @param rawDeviationCents The difference in cents between target and detected pitch.
     * @param retuneSpeedMs Retune speed in milliseconds (0 ms = hard tune snap).
     * @param strengthPct Correction strength (0% to 100%).
     * @param toleranceCents Pitch deadband tolerance in cents (0 to 100 cents).
     * @param humanizePct Humanize amount (0% to 100%).
     * @param flexTunePct Flex tune amount (0% to 100%).
     * @param vibratoPreservePct Vibrato preservation (0% to 100%).
     * @param mode Tuning character mode (Natural, Modern, HardTune, Robot).
     * @param numSamples Current block size for time-step smoothing.
     * @return Multiplicative pitch shift ratio (e.g. 1.0 = no shift, 1.05946 = +1 semitone).
     */
    float processBlock(float rawDeviationCents,
                       float retuneSpeedMs,
                       float strengthPct,
                       float toleranceCents,
                       float humanizePct,
                       float flexTunePct,
                       float vibratoPreservePct,
                       TuningModeType mode,
                       int numSamples)
    {
        // 1. Mode overrides / biasing
        float effectiveRetuneSpeed = retuneSpeedMs;
        float effectiveHumanize = humanizePct;

        switch (mode)
        {
            case TuningModeType::HardTune:
                effectiveRetuneSpeed = 0.0f;
                effectiveHumanize = 0.0f;
                break;
            case TuningModeType::Robot:
                effectiveRetuneSpeed = 0.0f;
                effectiveHumanize = 0.0f;
                break;
            case TuningModeType::Natural:
                effectiveRetuneSpeed = std::max(25.0f, retuneSpeedMs);
                effectiveHumanize = std::max(20.0f, humanizePct);
                break;
            case TuningModeType::Modern:
            default:
                break;
        }

        // 2. Deadband / Pitch Tolerance
        float targetShiftCents = rawDeviationCents;
        if (std::abs(targetShiftCents) <= toleranceCents)
        {
            targetShiftCents = 0.0f;
        }
        else if (targetShiftCents > 0.0f)
        {
            targetShiftCents -= toleranceCents;
        }
        else
        {
            targetShiftCents += toleranceCents;
        }

        // 3. Humanize Soft-Knee (preserves tiny micro-vocal inflections within ~15 cents)
        if (effectiveHumanize > 0.0f)
        {
            const float knee = (effectiveHumanize / 100.0f) * 18.0f;
            if (std::abs(targetShiftCents) < knee)
            {
                const float factor = std::pow(std::abs(targetShiftCents) / knee, 2.0f);
                targetShiftCents *= factor;
            }
        }

        // 4. Flex Tune (resists rapid transient pulls unless settled)
        if (flexTunePct > 0.0f)
        {
            const float diffFromPrev = std::abs(targetShiftCents - previousTargetCents);
            if (diffFromPrev > 60.0f) // Large pitch leap
            {
                const float flexDamping = 1.0f - (flexTunePct / 100.0f) * 0.7f;
                targetShiftCents *= flexDamping;
            }
        }
        previousTargetCents = targetShiftCents;

        // 5. Vibrato Preservation (extracts 4-7 Hz pitch modulation)
        if (vibratoPreservePct > 0.0f)
        {
            const float vibratoMod = rawDeviationCents - targetShiftCents;
            targetShiftCents += vibratoMod * (vibratoPreservePct / 100.0f) * 0.4f;
        }

        // 6. Correction Strength Scaling
        targetShiftCents *= (juce::jlimit(0.0f, 100.0f, strengthPct) / 100.0f);

        // 7. Retune Speed Smoothing Filter
        if (effectiveRetuneSpeed <= 0.5f)
        {
            // Instant snap (Hard Tune / Robot)
            smoothedPitchShiftCents = targetShiftCents;
        }
        else
        {
            // Single-pole low-pass filter: alpha = 1 - exp(-dt / tau)
            const float dtSeconds = static_cast<float>(numSamples) / static_cast<float>(sampleRate);
            const float tauSeconds = effectiveRetuneSpeed * 0.001f;
            const float alpha = 1.0f - std::exp(-dtSeconds / std::max(1e-4f, tauSeconds));
            smoothedPitchShiftCents += alpha * (targetShiftCents - smoothedPitchShiftCents);
        }

        // 8. Convert cents to pitch transposition ratio
        // ratio = 2^(cents / 1200)
        const float pitchRatio = std::pow(2.0f, smoothedPitchShiftCents / 1200.0f);
        return juce::jlimit(0.25f, 4.0f, pitchRatio);
    }

    float getSmoothedShiftCents() const noexcept { return smoothedPitchShiftCents; }

private:
    double sampleRate { 44100.0 };
    float smoothedPitchShiftCents { 0.0f };
    float previousTargetCents { 0.0f };
    float vibratoBuffer { 0.0f };
};

} // namespace toytune
