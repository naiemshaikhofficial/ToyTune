#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace toytune
{

namespace ParameterIDs
{
    inline constexpr const char* inputGain         = "input_gain";
    inline constexpr const char* vocalRange        = "vocal_range";
    inline constexpr const char* rootNote          = "root_note";
    inline constexpr const char* scaleType         = "scale_type";
    inline constexpr const char* referencePitch    = "reference_pitch";
    inline constexpr const char* retuneSpeed       = "retune_speed";
    inline constexpr const char* noteTransition    = "note_transition";
    inline constexpr const char* correctionStrength= "correction_strength";
    inline constexpr const char* pitchTolerance    = "pitch_tolerance";
    inline constexpr const char* humanize          = "humanize";
    inline constexpr const char* flexTune          = "flex_tune";
    inline constexpr const char* vibratoPreserve   = "vibrato_preserve";
    inline constexpr const char* tuningMode        = "tuning_mode";
    inline constexpr const char* formantPreserve   = "formant_preserve";
    inline constexpr const char* formantShift      = "formant_shift";
    inline constexpr const char* lowLatencyMode    = "low_latency_mode";
    inline constexpr const char* dryWetMix         = "dry_wet_mix";
    inline constexpr const char* outputGain        = "output_gain";
    inline constexpr const char* bypass            = "bypass";

    // 12 semitones note masks: note_mask_0 (C) to note_mask_11 (B)
    inline juce::String noteMask(int noteIndex)
    {
        return "note_mask_" + juce::String(noteIndex);
    }
}

enum class VocalRangeType
{
    Wide = 0,
    BassBaritone,
    Tenor,
    AltoSoprano
};

enum class TuningModeType
{
    Natural = 0,
    Modern,
    HardTune,
    Robot
};

class ToyTuneParameters
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    static const juce::StringArray& getRootNoteNames();
    static const juce::StringArray& getScaleTypeNames();
    static const juce::StringArray& getVocalRangeNames();
    static const juce::StringArray& getTuningModeNames();
};

} // namespace toytune
