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

        // 0. If Retune Speed is 0, completely bypass pitch shift (clean original dry voice)
        if (retuneSpeedMs <= 0.001f)
        {
            stage1Cents = 0.0f;
            smoothedPitchShiftCents = 0.0f;
            return 1.0f;
        }

        const float speedNorm = juce::jlimit(0.0f, 100.0f, retuneSpeedMs) / 100.0f;

        // Inverted response curve:
        // 100 on knob -> 0.0 ms (instant hard tune snap / maximum auto-tune)
        // 90 on knob  -> ~2.8 ms (very fast pop autotune)
        // 75 on knob  -> ~17.5 ms (modern pop vocal tune)
        // 50 on knob  -> ~70 ms (natural melodious vocal)
        // 20 on knob  -> ~180 ms (subtle gentle correction)
        const float inv = 1.0f - speedNorm;
        float effectiveRetuneSpeed = inv * inv * 280.0f;
        float effectiveHumanize = humanizePct;
        float effectiveFlexTune = flexTunePct;

        // 1. Mode overrides / biasing
        switch (mode)
        {
            case TuningModeType::HardTune:
            case TuningModeType::Robot:
                effectiveRetuneSpeed = 0.0f;
                effectiveHumanize = 0.0f;
                effectiveFlexTune = 0.0f;
                break;
            case TuningModeType::Natural: // Classic Mode (Auto-Tune 5 architecture)
                // Flex-Tune was not in Auto-Tune 5, so it is strictly disabled in Classic mode
                effectiveFlexTune = 0.0f;
                break;
            case TuningModeType::Modern:
            default:
                break;
        }

        // 2. Deadband / Pitch Tolerance & Intensity Scaling
        float targetShiftCents = rawDeviationCents * std::min(1.0f, speedNorm * 1.05f);
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

        // 4. Flex Tune (resists rapid transient pulls unless settled - bypassed in Classic mode)
        if (effectiveFlexTune > 0.0f)
        {
            const float diffFromPrev = std::abs(targetShiftCents - previousTargetCents);
            if (diffFromPrev > 60.0f) // Large pitch leap
            {
                const float flexDamping = 1.0f - (effectiveFlexTune / 100.0f) * 0.7f;
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
        // Discretized via exact analytical 2-pole cascaded filter (unconditionally stable for all tau >= 0)
        targetShiftCents = juce::jlimit(-400.0f, 400.0f, targetShiftCents);

        if (effectiveRetuneSpeed <= 0.1f)
        {
            // Instant snap: zero delay, zero lag (Hard Tune / Travis Scott effect)
            stage1Cents = targetShiftCents;
            smoothedPitchShiftCents = targetShiftCents;
        }
        else
        {
            // Two identical cascaded 1st-order stages provide critically damped (Q = 0.5)
            // S-curve step response with zero overshoot and 100% unconditional stability!
            const float tau = effectiveRetuneSpeed * 0.001f;
            const float tauStage = std::max(1e-4f, tau * 0.5f);
            const float alpha = 1.0f - std::exp(-dtSeconds / tauStage);

            stage1Cents += alpha * (targetShiftCents - stage1Cents);
            smoothedPitchShiftCents += alpha * (stage1Cents - smoothedPitchShiftCents);

            if (!std::isfinite(smoothedPitchShiftCents) || !std::isfinite(stage1Cents))
            {
                stage1Cents = targetShiftCents;
                smoothedPitchShiftCents = targetShiftCents;
            }
        }

        smoothedPitchShiftCents = juce::jlimit(-400.0f, 400.0f, smoothedPitchShiftCents);

        // 9. Convert cents to pitch transposition ratio: ratio = 2^(cents / 1200)
        float pitchRatio = std::pow(2.0f, smoothedPitchShiftCents / 1200.0f);
        if (!std::isfinite(pitchRatio) || pitchRatio <= 0.0f)
            pitchRatio = 1.0f;

        return juce::jlimit(0.25f, 4.0f, pitchRatio);
    }

    float getSmoothedShiftCents() const noexcept { return smoothedPitchShiftCents; }

    void reset()
    {
        stage1Cents = 0.0f;
        smoothedPitchShiftCents = 0.0f;
        previousTargetCents = 0.0f;
        lastRawDeviation = 0.0f;
        heldNoteDurationMs = 0.0f;
        bq_s1 = 0.0f;
        bq_s2 = 0.0f;
    }

private:
    double sampleRate { 44100.0 };
    float smoothedPitchShiftCents { 0.0f };
    float stage1Cents { 0.0f };
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
