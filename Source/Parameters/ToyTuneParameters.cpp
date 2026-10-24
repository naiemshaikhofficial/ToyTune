#include "ToyTuneParameters.h"

namespace toytune
{

const juce::StringArray& ToyTuneParameters::getRootNoteNames()
{
    static const juce::StringArray names { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names;
}

const juce::StringArray& ToyTuneParameters::getScaleTypeNames()
{
    static const juce::StringArray names {
        "Chromatic",
        "Major",
        "Natural Minor",
        "Harmonic Minor",
        "Melodic Minor",
        "Pentatonic Major",
        "Pentatonic Minor",
        "Blues",
        "Custom"
    };
    return names;
}

const juce::StringArray& ToyTuneParameters::getVocalRangeNames()
{
    static const juce::StringArray names { "Wide (All Voices)", "Bass / Baritone", "Tenor", "Alto / Soprano" };
    return names;
}

const juce::StringArray& ToyTuneParameters::getTuningModeNames()
{
    static const juce::StringArray names { "Natural", "Modern", "Hard Tune", "Robot" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout ToyTuneParameters::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. Input Section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::inputGain, 1 },
        "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::vocalRange, 1 },
        "Vocal Range",
        getVocalRangeNames(),
        0));

    // 2. Key & Scale Section
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::rootNote, 1 },
        "Root Key",
        getRootNoteNames(),
        0)); // C default

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::scaleType, 1 },
        "Scale Type",
        getScaleTypeNames(),
        0)); // Chromatic default

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::referencePitch, 1 },
        "Reference A4",
        juce::NormalisableRange<float>(415.0f, 465.0f, 0.1f),
        440.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // 12 Semitone Note Masks (default all true for chromatic)
    for (int i = 0; i < 12; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { ParameterIDs::noteMask(i), 1 },
            "Note " + getRootNoteNames()[i],
            true));
    }

    // 12 Semitone Note Bypass (default false)
    for (int i = 0; i < 12; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { ParameterIDs::noteBypass(i), 1 },
            "Bypass " + getRootNoteNames()[i],
            false));
    }

    // 12 Semitone Cents Detune (-50 to +50 cents, default 0)
    for (int i = 0; i < 12; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParameterIDs::noteCents(i), 1 },
            "Detune " + getRootNoteNames()[i],
            juce::NormalisableRange<float>(-50.0f, 50.0f, 1.0f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("cents")));
    }

    // Advanced LFO Vibrato (Auto-Tune Pro Create Vibrato)
    static const juce::StringArray vibShapeNames { "Off", "Sine", "Square", "Saw" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::advVibShape, 1 },
        "Vibrato Shape",
        vibShapeNames,
        0)); // Off default

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::advVibRate, 1 },
        "Vibrato Rate",
        juce::NormalisableRange<float>(1.0f, 12.0f, 0.1f),
        5.5f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::advVibDelay, 1 },
        "Vibrato Delay",
        juce::NormalisableRange<float>(0.0f, 1500.0f, 10.0f),
        400.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::advVibOnset, 1 },
        "Vibrato Onset",
        juce::NormalisableRange<float>(50.0f, 1500.0f, 10.0f),
        400.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::advVibVariation, 1 },
        "Vibrato Variation",
        juce::NormalisableRange<float>(0.0f, 50.0f, 1.0f),
        15.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::advVibPitch, 1 },
        "Vibrato Pitch Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("cents")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::advVibAmp, 1 },
        "Vibrato Amplitude Depth",
        juce::NormalisableRange<float>(0.0f, 30.0f, 0.5f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::advVibFormant, 1 },
        "Vibrato Formant Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Advanced Drawer UI State
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::advDrawerOpen, 1 },
        "Advanced Drawer Open",
        false));

    static const juce::StringArray tabNames { "Vibrato", "Scale" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::advActiveTab, 1 },
        "Advanced Active Tab",
        tabNames,
        1));

    // 3. Correction Section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::retuneSpeed, 1 },
        "Retune Speed",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::correctionStrength, 1 },
        "Correction Strength",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::pitchTolerance, 1 },
        "Pitch Tolerance",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("cents")));

    // 4. Expression Section
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::humanize, 1 },
        "Humanize",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::flexTune, 1 },
        "Flex Tune",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::vibratoPreserve, 1 },
        "Vibrato Preserve",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // 5. Modes
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::tuningMode, 1 },
        "Tuning Character Mode",
        getTuningModeNames(),
        1)); // Modern default

    // 6. Voice Character / Formants
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::formantPreserve, 1 },
        "Formant Preserve",
        true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::formantShift, 1 },
        "Formant Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    // 7. Output Section & Quality
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::lowLatencyMode, 1 },
        "Low Latency Mode",
        false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::dryWetMix, 1 },
        "Dry / Wet Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::outputGain, 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::bypass, 1 },
        "Bypass",
        false));

    // 8. Harmony Player Section (Auto-Tune Pro 11 4-Voice Real-Time Harmonizer)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::harmonyEnable, 1 },
        "Harmony Player Enable",
        false));

    static const juce::StringArray harmModes { "Scale Diatonic", "Fixed Semitones", "MIDI Notes" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::harmonyMode, 1 },
        "Harmony Mode",
        harmModes,
        0));

    static const juce::StringArray harmIntervalNames {
        "-12 (Oct Down)", "-10 (7th Down)", "-7 (5th Down)", "-5 (4th Down)", "-3/-4 (3rd Down)",
        "0 (Unison)",
        "+3/+4 (3rd Up)", "+5 (4th Up)", "+7 (5th Up)", "+10 (7th Up)", "+12 (Oct Up)"
    };

    const float defaultPans[4] = { -0.6f, 0.6f, -0.3f, 0.3f };
    const int defaultIntervals[4] = { 6 /* +3rd */, 8 /* +5th */, 4 /* -3rd */, 0 /* -12 */ };

    for (int i = 0; i < 4; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { ParameterIDs::harmVoiceEnable(i), 1 },
            "Harmony Voice " + juce::String(i + 1) + " Enable",
            i < 2)); // Voice 1 & 2 enabled by default

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { ParameterIDs::harmVoiceInterval(i), 1 },
            "Harmony Voice " + juce::String(i + 1) + " Interval",
            harmIntervalNames,
            defaultIntervals[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParameterIDs::harmVoiceFormant(i), 1 },
            "Harmony Voice " + juce::String(i + 1) + " Formant",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("st")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParameterIDs::harmVoicePan(i), 1 },
            "Harmony Voice " + juce::String(i + 1) + " Pan",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
            defaultPans[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParameterIDs::harmVoiceLevel(i), 1 },
            "Harmony Voice " + juce::String(i + 1) + " Level",
            juce::NormalisableRange<float>(-36.0f, 6.0f, 0.1f),
            -6.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { ParameterIDs::harmVoiceSolo(i), 1 },
            "Harmony Voice " + juce::String(i + 1) + " Solo",
            false));
    }

    // Humanize & Realism
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmNaturalize, 1 },
        "Harmony Naturalize",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f),
        25.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmTransition, 1 },
        "Harmony Transition Time",
        juce::NormalisableRange<float>(0.0f, 500.0f, 5.0f),
        125.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmPitchVar, 1 },
        "Harmony Pitch Variation",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.5f),
        20.0f,
        juce::AudioParameterFloatAttributes().withLabel("cents")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmTimingVar, 1 },
        "Harmony Timing Variation",
        juce::NormalisableRange<float>(0.0f, 80.0f, 1.0f),
        35.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Dynamics, EQ & Mastering Strip
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmEnvAttack, 1 },
        "Harmony Attack",
        juce::NormalisableRange<float>(0.0f, 500.0f, 1.0f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmEnvRelease, 1 },
        "Harmony Release",
        juce::NormalisableRange<float>(5.0f, 1500.0f, 5.0f),
        150.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmEqFreq, 1 },
        "Harmony High-Pass EQ",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f),
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmGateThresh, 1 },
        "Harmony Gate Threshold",
        juce::NormalisableRange<float>(-80.0f, 0.0f, 0.5f),
        -55.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::harmStereoWidth, 1 },
        "Harmony Stereo Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f),
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

} // namespace toytune
