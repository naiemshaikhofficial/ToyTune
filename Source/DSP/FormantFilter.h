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
        leftFilter.prepare(spec);
        rightFilter.prepare(spec);
        reset();
    }

    void reset()
    {
        leftFilter.reset();
        rightFilter.reset();
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
        else if (formantPreserveEnabled && (pitchShiftRatio > 1.3f || pitchShiftRatio < 0.75f))
        {
            // Only apply mild compensation for extreme transpositions (> 4 semitones)
            targetFormantRatio = juce::jlimit(0.8f, 1.25f, 1.0f / pitchShiftRatio);
        }

        targetFormantRatio = juce::jlimit(0.5f, 2.0f, targetFormantRatio);

        // Smooth filter ratio to prevent stepping
        currentFormantRatio = 0.95f * currentFormantRatio + 0.05f * targetFormantRatio;

        if (std::abs(currentFormantRatio - 1.0f) > 0.02f)
        {
            const float centerFreq = juce::jlimit(200.0f, static_cast<float>(sampleRate * 0.45), 1400.0f * currentFormantRatio);
            const float gainDb = (currentFormantRatio - 1.0f) * 4.0f;

            auto leftCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, centerFreq, 1.0f, juce::Decibels::decibelsToGain(gainDb));
            auto rightCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, centerFreq, 1.0f, juce::Decibels::decibelsToGain(gainDb));

            *leftFilter.state = *leftCoeffs;
            *rightFilter.state = *rightCoeffs;
        }
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (std::abs(currentFormantRatio - 1.0f) <= 0.02f)
            return; // 100% clean, transparent bypass

        juce::dsp::AudioBlock<float> block(buffer);
        if (buffer.getNumChannels() >= 1)
        {
            auto leftBlock = block.getSingleChannelBlock(0);
            juce::dsp::ProcessContextReplacing<float> contextL(leftBlock);
            leftFilter.process(contextL);
        }
        if (buffer.getNumChannels() >= 2)
        {
            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> contextR(rightBlock);
            rightFilter.process(contextR);
        }
    }

private:
    double sampleRate { 44100.0 };
    float currentFormantRatio { 1.0f };

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> leftFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> rightFilter;
};

} // namespace toytune
