#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace toytune
{

/**
 * Clean Formant Shifter & Compensation Module.
 * Period-synchronous time-domain pitch shifting inherently preserves the formant envelope
 * without requiring comb/phase-modulating filters. This module provides manual formant shifting
 * (-12 to +12 semitones) when the user explicitly engages it.
 */
class FormantFilter
{
public:
    FormantFilter() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        f1FilterL.prepare(spec);
        f1FilterR.prepare(spec);
        f2FilterL.prepare(spec);
        f2FilterR.prepare(spec);
        reset();
    }

    void reset()
    {
        f1FilterL.reset();
        f1FilterR.reset();
        f2FilterL.reset();
        f2FilterR.reset();
        currentFormantRatio = 1.0f;
    }

    void updateParameters(float pitchShiftRatio, float formantShiftSemitones, bool formantPreserveEnabled)
    {
        float targetFormantRatio = 1.0f;

        // Manual formant shift takes priority
        if (std::abs(formantShiftSemitones) > 0.1f)
        {
            targetFormantRatio = std::pow(2.0f, formantShiftSemitones / 12.0f);
        }
        else if (formantPreserveEnabled && (pitchShiftRatio > 1.06f || pitchShiftRatio < 0.94f))
        {
            // Subtle acoustic throat compensation when pitch shifts across semitones
            targetFormantRatio = juce::jlimit(0.85f, 1.18f, 1.0f / std::pow(pitchShiftRatio, 0.75f));
        }

        targetFormantRatio = juce::jlimit(0.5f, 2.0f, targetFormantRatio);

        // Smooth filter ratio to eliminate stepping artifacts
        currentFormantRatio = 0.96f * currentFormantRatio + 0.04f * targetFormantRatio;

        if (std::abs(currentFormantRatio - 1.0f) > 0.015f)
        {
            const float ratioDev = currentFormantRatio - 1.0f;
            const float nyquist = static_cast<float>(sampleRate * 0.45);

            // F1: Pharyngeal / Throat Chest Formant (~680 Hz)
            const float f1Center = juce::jlimit(200.0f, nyquist, 680.0f * currentFormantRatio);
            const float f1GainDb = ratioDev * 3.5f;

            auto f1Coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, f1Center, 1.4f, juce::Decibels::decibelsToGain(f1GainDb));
            *f1FilterL.state = *f1Coeffs;
            *f1FilterR.state = *f1Coeffs;

            // F2: Oral Cavity / Vowel Core Formant (~1950 Hz)
            const float f2Center = juce::jlimit(400.0f, nyquist, 1950.0f * currentFormantRatio);
            const float f2GainDb = ratioDev * 2.8f;

            auto f2Coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, f2Center, 1.8f, juce::Decibels::decibelsToGain(f2GainDb));
            *f2FilterL.state = *f2Coeffs;
            *f2FilterR.state = *f2Coeffs;
        }
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (std::abs(currentFormantRatio - 1.0f) <= 0.015f)
            return; // 100% clean, pristine transparent bypass

        juce::dsp::AudioBlock<float> block(buffer);
        if (buffer.getNumChannels() >= 1)
        {
            auto leftBlock = block.getSingleChannelBlock(0);
            juce::dsp::ProcessContextReplacing<float> contextL(leftBlock);
            f1FilterL.process(contextL);
            f2FilterL.process(contextL);
        }
        if (buffer.getNumChannels() >= 2)
        {
            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> contextR(rightBlock);
            f1FilterR.process(contextR);
            f2FilterR.process(contextR);
        }
    }

private:
    double sampleRate { 44100.0 };
    float currentFormantRatio { 1.0f };

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> f1FilterL;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> f1FilterR;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> f2FilterL;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> f2FilterR;
};

} // namespace toytune
