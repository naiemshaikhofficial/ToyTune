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
        updateBiquadCoefficients();
        reset();
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
        if (!std::isfinite(rawDeviationCents))
            rawDeviationCents = 0.0f;

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

        // 5. Advanced Vibrato Preservation (Unconditionally Stable Biquad Bandpass)
        const float dtSeconds = static_cast<float>(numSamples) / static_cast<float>(sampleRate);
        if (vibratoPreservePct > 0.0f)
        {
            const float bpOutput = updateVibratoBandpass(rawDeviationCents);
            targetShiftCents += bpOutput * (vibratoPreservePct / 100.0f);
        }

        // 6. Sustained Note Humanizer (differentiates fast melisma runs from held notes)
        if (effectiveHumanize > 0.0f)
        {
            if (std::abs(rawDeviationCents - lastRawDeviation) < 40.0f)
            {
                heldNoteDurationMs += dtSeconds * 1000.0f;
            }
            else
            {
                heldNoteDurationMs = 0.0f;
            }
            lastRawDeviation = rawDeviationCents;

            if (heldNoteDurationMs > 150.0f)
            {
                const float relax = (effectiveHumanize / 100.0f) * std::min(45.0f, (heldNoteDurationMs - 150.0f) * 0.1f);
                effectiveRetuneSpeed += relax;
            }
        }
        else
        {
            heldNoteDurationMs = 0.0f;
            lastRawDeviation = rawDeviationCents;
        }

        // 7. Correction Strength Scaling
        targetShiftCents *= (juce::jlimit(0.0f, 100.0f, strengthPct) / 100.0f);

        // 8. Critically Damped 2nd-Order S-Curve Retune Ballistics (Auto-Tune / Waves Tune standard)
        if (effectiveRetuneSpeed <= 0.5f)
        {
            // Instant snap: zero delay, zero lag (Hard Tune / Travis Scott effect)
            smoothedPitchShiftCents = targetShiftCents;
            pitchVelocity = 0.0f;
        }
        else
        {
            const float tau = std::max(0.001f, effectiveRetuneSpeed * 0.001f);
            const float omega = 4.5f / tau;
            const float error = targetShiftCents - smoothedPitchShiftCents;
            const float accel = (omega * omega * error) - (2.0f * omega * pitchVelocity);

            pitchVelocity += accel * dtSeconds;
            smoothedPitchShiftCents += pitchVelocity * dtSeconds;

            if (!std::isfinite(smoothedPitchShiftCents) || !std::isfinite(pitchVelocity))
            {
                smoothedPitchShiftCents = targetShiftCents;
                pitchVelocity = 0.0f;
            }
        }

        smoothedPitchShiftCents = juce::jlimit(-2400.0f, 2400.0f, smoothedPitchShiftCents);

        // 9. Convert cents to pitch transposition ratio: ratio = 2^(cents / 1200)
        float pitchRatio = std::pow(2.0f, smoothedPitchShiftCents / 1200.0f);
        if (!std::isfinite(pitchRatio) || pitchRatio <= 0.0f)
            pitchRatio = 1.0f;

        return juce::jlimit(0.25f, 4.0f, pitchRatio);
    }

    float getSmoothedShiftCents() const noexcept { return smoothedPitchShiftCents; }

    void reset()
    {
        smoothedPitchShiftCents = 0.0f;
        pitchVelocity = 0.0f;
        previousTargetCents = 0.0f;
        lastRawDeviation = 0.0f;
        heldNoteDurationMs = 0.0f;
        bq_s1 = 0.0f;
        bq_s2 = 0.0f;
    }

private:
    double sampleRate { 44100.0 };
    float smoothedPitchShiftCents { 0.0f };
    float pitchVelocity { 0.0f };
    float previousTargetCents { 0.0f };
    float lastRawDeviation { 0.0f };
    float heldNoteDurationMs { 0.0f };

    // Unconditionally stable Direct Form II Biquad Bandpass Filter (5.5 Hz)
    float bq_b0 { 0.0f };
    float bq_b1 { 0.0f };
    float bq_b2 { 0.0f };
    float bq_a1 { 0.0f };
    float bq_a2 { 0.0f };
    float bq_s1 { 0.0f };
    float bq_s2 { 0.0f };

    void updateBiquadCoefficients()
    {
        const double f0 = 5.5;
        const double Q = 1.4;
        const double w0 = 2.0 * juce::MathConstants<double>::pi * (f0 / sampleRate);
        const double alpha = std::sin(w0) / (2.0 * Q);
        const double cosw0 = std::cos(w0);

        const double a0 = 1.0 + alpha;
        bq_b0 = static_cast<float>(alpha / a0);
        bq_b1 = 0.0f;
        bq_b2 = static_cast<float>(-alpha / a0);
        bq_a1 = static_cast<float>((-2.0 * cosw0) / a0);
        bq_a2 = static_cast<float>((1.0 - alpha) / a0);
    }

    float updateVibratoBandpass(float input)
    {
        if (!std::isfinite(input))
            return 0.0f;

        const float out = bq_b0 * input + bq_s1;
        bq_s1 = bq_b1 * input - bq_a1 * out + bq_s2;
        bq_s2 = bq_b2 * input - bq_a2 * out;

        if (!std::isfinite(out) || !std::isfinite(bq_s1) || !std::isfinite(bq_s2))
        {
            bq_s1 = 0.0f;
            bq_s2 = 0.0f;
            return 0.0f;
        }

        return juce::jlimit(-100.0f, 100.0f, out);
    }
};

} // namespace toytune
