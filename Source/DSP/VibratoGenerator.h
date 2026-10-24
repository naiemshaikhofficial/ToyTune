#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>

namespace toytune
{

enum class VibratoShape
{
    Off = 0,
    Sine,
    Square,
    Sawtooth
};

struct VibratoModulationOutput
{
    float pitchModCents { 0.0f };
    float ampModGain { 1.0f };
    float formantModRatio { 1.0f };
};

/**
 * Studio-Grade Synthesized LFO Vocal Vibrato Generator (Auto-Tune Architecture).
 * Features Delay, Onset Ramp, Humanized Variation, and Multi-Target Modulation
 * (Pitch, Amplitude, Formant). Zero heap allocation in audio thread.
 */
class VibratoGenerator
{
public:
    VibratoGenerator() = default;

    void prepare(double newSampleRate)
    {
        sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        phase = 0.0f;
        heldTimerSeconds = 0.0f;
        smoothedDepth = 0.0f;
        randomPhaseJitter = 0.0f;
    }

    /**
     * Process modulation for a sub-hop block of samples.
     * @param shape LFO waveform (Off, Sine, Square, Sawtooth)
     * @param rateHz Vibrato frequency (4.0 to 9.0 Hz)
     * @param delayMs Milliseconds before vibrato begins on a held note
     * @param onsetMs Fade-in ramp duration in milliseconds
     * @param variationPct Humanized random variation (0 to 50%)
     * @param modPitchCents Depth of pitch modulation in cents (0 to 100 ct)
     * @param modAmpPct Depth of tremolo amplitude modulation in % (0 to 30%)
     * @param modFormantPct Depth of throat formant resonance modulation (0 to 100%)
     * @param isVoiced Whether singer is actively vocalizing
     * @param numSamples Number of samples in this sub-hop
     */
    VibratoModulationOutput processBlock(VibratoShape shape,
                                         float rateHz,
                                         float delayMs,
                                         float onsetMs,
                                         float variationPct,
                                         float modPitchCents,
                                         float modAmpPct,
                                         float modFormantPct,
                                         bool isVoiced,
                                         int numSamples)
    {
        VibratoModulationOutput out;
        if (shape == VibratoShape::Off || (modPitchCents <= 0.01f && modAmpPct <= 0.01f && modFormantPct <= 0.01f))
        {
            smoothedDepth = 0.0f;
            return out;
        }

        const float dt = static_cast<float>(numSamples) / static_cast<float>(sampleRate);

        // 1. Sustained Note Tracking
        if (isVoiced)
        {
            heldTimerSeconds += dt;
        }
        else
        {
            // Rapid decay on silence/breath
            heldTimerSeconds = std::max(0.0f, heldTimerSeconds - dt * 3.0f);
        }

        // 2. Envelope State (Delay -> Onset Ramp -> Sustain)
        const float delaySec = std::max(0.0f, delayMs * 0.001f);
        const float onsetSec = std::max(0.01f, onsetMs * 0.001f);

        float targetDepth = 0.0f;
        if (heldTimerSeconds > delaySec)
        {
            const float progress = (heldTimerSeconds - delaySec) / onsetSec;
            targetDepth = juce::jlimit(0.0f, 1.0f, progress);
            // Smooth S-curve onset
            targetDepth = targetDepth * targetDepth * (3.0f - 2.0f * targetDepth);
        }

        // Smooth depth transitions to avoid sudden thumps
        const float alpha = 1.0f - std::exp(-dt / 0.025f);
        smoothedDepth += alpha * (targetDepth - smoothedDepth);

        if (smoothedDepth <= 1e-4f)
            return out;

        // 3. Humanized Variation Jitter (Random frequency & phase micro-drift)
        const float varNorm = juce::jlimit(0.0f, 50.0f, variationPct) / 100.0f;
        if (varNorm > 0.001f)
        {
            // Subtle pseudo-random drift without rand() calls in tight loop
            randomPhaseJitter += (std::sin(phase * 0.37f) * 0.05f) * varNorm;
            randomPhaseJitter = juce::jlimit(-0.25f, 0.25f, randomPhaseJitter);
        }
        else
        {
            randomPhaseJitter = 0.0f;
        }

        // 4. LFO Oscillator Synthesis
        const float effectiveRate = juce::jlimit(1.0f, 15.0f, rateHz * (1.0f + randomPhaseJitter * 0.2f));
        const float phaseStep = effectiveRate * dt;
        phase += phaseStep;
        if (phase >= 1.0f)
            phase -= 1.0f;

        float lfo = 0.0f;
        switch (shape)
        {
            case VibratoShape::Sine:
                lfo = std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
                break;

            case VibratoShape::Square:
                lfo = (phase < 0.5f) ? 1.0f : -1.0f;
                break;

            case VibratoShape::Sawtooth:
                lfo = (2.0f * phase) - 1.0f;
                break;

            case VibratoShape::Off:
            default:
                lfo = 0.0f;
                break;
        }

        // 5. Multi-Target Modulation Routing
        const float modAmount = lfo * smoothedDepth;

        // Target A: Pitch Modulation (Cents)
        out.pitchModCents = modAmount * juce::jlimit(0.0f, 100.0f, modPitchCents);

        // Target B: Amplitude Modulation (Tremolo)
        const float ampDepth = (juce::jlimit(0.0f, 30.0f, modAmpPct) / 100.0f) * 0.5f;
        out.ampModGain = juce::jlimit(0.4f, 1.6f, 1.0f + (modAmount * ampDepth));

        // Target C: Formant Modulation (Larynx resonance undulation)
        const float formantDepth = (juce::jlimit(0.0f, 100.0f, modFormantPct) / 100.0f) * 0.12f;
        out.formantModRatio = juce::jlimit(0.85f, 1.15f, 1.0f + (modAmount * formantDepth));

        return out;
    }

private:
    double sampleRate { 44100.0 };
    float phase { 0.0f };
    float heldTimerSeconds { 0.0f };
    float smoothedDepth { 0.0f };
    float randomPhaseJitter { 0.0f };
};

} // namespace toytune
