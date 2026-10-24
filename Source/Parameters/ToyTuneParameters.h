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

    // 12 semitones note bypass: note_byp_0 (C) to note_byp_11 (B)
    inline juce::String noteBypass(int noteIndex)
    {
        return "note_byp_" + juce::String(noteIndex);
    }

    // 12 semitones cents detune: note_cents_0 (C) to note_cents_11 (B)
    inline juce::String noteCents(int noteIndex)
    {
        return "note_cents_" + juce::String(noteIndex);
    }

    // Advanced Vibrato (Create Vibrato LFO)
    inline constexpr const char* advVibShape       = "adv_vib_shape";
    inline constexpr const char* advVibRate        = "adv_vib_rate";
    inline constexpr const char* advVibDelay       = "adv_vib_delay";
    inline constexpr const char* advVibOnset       = "adv_vib_onset";
    inline constexpr const char* advVibVariation   = "adv_vib_variation";
    inline constexpr const char* advVibPitch       = "adv_vib_pitch";
    inline constexpr const char* advVibAmp         = "adv_vib_amp";
    inline constexpr const char* advVibFormant     = "adv_vib_formant";

    // Advanced Drawer UI State
    inline constexpr const char* advDrawerOpen     = "adv_drawer_open";
    inline constexpr const char* advActiveTab      = "adv_active_tab";

    // Harmony Player IDs (Auto-Tune Pro 11 4-Voice Real-Time Harmonizer)
    inline constexpr const char* harmonyEnable     = "harmony_enable";
    inline constexpr const char* harmonyMode       = "harmony_mode"; // 0=Scale Diatonic, 1=Fixed, 2=MIDI

    inline juce::String harmVoiceEnable(int v)   { return "harm_v_en_" + juce::String(v); }
    inline juce::String harmVoiceInterval(int v) { return "harm_v_int_" + juce::String(v); }
    inline juce::String harmVoiceFormant(int v)  { return "harm_v_fmt_" + juce::String(v); }
    inline juce::String harmVoicePan(int v)      { return "harm_v_pan_" + juce::String(v); }
    inline juce::String harmVoiceLevel(int v)    { return "harm_v_lvl_" + juce::String(v); }
    inline juce::String harmVoiceSolo(int v)     { return "harm_v_solo_" + juce::String(v); }

    // Harmony Humanize & Realism
    inline constexpr const char* harmNaturalize    = "harm_naturalize";
    inline constexpr const char* harmTransition    = "harm_transition";
    inline constexpr const char* harmPitchVar      = "harm_pitch_var";
    inline constexpr const char* harmTimingVar     = "harm_timing_var";

    // Harmony Dynamics, EQ & Mastering Strip
    inline constexpr const char* harmEnvAttack     = "harm_env_attack";
    inline constexpr const char* harmEnvRelease    = "harm_env_release";
    inline constexpr const char* harmEqFreq        = "harm_eq_freq";
    inline constexpr const char* harmGateThresh    = "harm_gate_thresh";
    inline constexpr const char* harmStereoWidth   = "harm_stereo_width";
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
