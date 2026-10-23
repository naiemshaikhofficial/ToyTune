#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace toytune
{

/**
 * Real-time formant envelope compensator and formant shifter.
 * Counteracts the vocal chipmunk/deepening effect during pitch modification
 * using resonant formant correction biquad stages.
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

    /**
     * Updates formant compensation based on pitch shift ratio and user formant shift.
     * @param pitchShiftRatio The ratio of pitch modification (e.g. 1.059 = +1 st).
     * @param formantShiftSemitones User manual formant shift (-12 to +12 st).
     * @param formantPreserveEnabled Whether automatic formant envelope preservation is active.
     */
    void updateParameters(float pitchShiftRatio, float formantShiftSemitones, bool formantPreserveEnabled)
    {
        // Total desired formant shift ratio
        float targetFormantRatio = 1.0f;

        if (formantPreserveEnabled && pitchShiftRatio > 0.1f)
        {
            // Invert the pitch shift to keep formant envelope stationary
            targetFormantRatio = 1.0f / pitchShiftRatio;
        }

        // Apply manual formant shift
        if (std::abs(formantShiftSemitones) > 0.05f)
        {
            targetFormantRatio *= std::pow(2.0f, formantShiftSemitones / 12.0f);
        }

        targetFormantRatio = juce::jlimit(0.5f, 2.0f, targetFormantRatio);

        // Smooth filter ratio
        currentFormantRatio = 0.9f * currentFormantRatio + 0.1f * targetFormantRatio;

        // Base vocal formant center frequencies: F1 ~ 750 Hz, F2 ~ 1800 Hz
        const float centerFreq = juce::jlimit(200.0f, static_cast<float>(sampleRate * 0.45), 1400.0f * currentFormantRatio);
        const float gainDb = (currentFormantRatio - 1.0f) * 6.0f; // Modest boost/cut

        auto leftCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, centerFreq, 1.2f, juce::Decibels::decibelsToGain(gainDb));
        auto rightCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, centerFreq, 1.2f, juce::Decibels::decibelsToGain(gainDb));

        *leftFilter.state = *leftCoeffs;
        *rightFilter.state = *rightCoeffs;
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (std::abs(currentFormantRatio - 1.0f) < 0.01f)
            return; // Neutral bypass

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
