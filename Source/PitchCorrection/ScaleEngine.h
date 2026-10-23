#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

namespace toytune
{

struct TargetPitchResult
{
    float targetMidiNote { 60.0f };
    float targetFreqHz { 261.63f };
    float centsDeviation { 0.0f }; // In cents: positive = flat needs pulling up, negative = sharp needs pulling down
    int   targetSemitone { 0 };    // 0 to 11
    int   targetOctave { 4 };
    juce::String noteName;
};

class ScaleEngine
{
public:
    ScaleEngine();

    void setRootNote(int rootIndex); // 0 = C, 11 = B
    void setScaleType(int scaleTypeIndex);
    void setCustomNoteActive(int semitoneIndex, bool isActive);
    void setCustomMask(const std::array<bool, 12>& mask);
    void setReferencePitch(float refHz);

    int getRootNote() const noexcept { return rootNote; }
    int getScaleType() const noexcept { return scaleType; }
    float getReferencePitch() const noexcept { return referenceA4; }
    bool isSemitoneActive(int semitone) const noexcept;

    std::array<bool, 12> getActiveMask() const;

    /**
     * Calculates the nearest target note in the active scale for a given fractional MIDI note.
     */
    TargetPitchResult calculateTarget(float inputMidiNote) const;

    static juce::String getNoteNameWithOctave(int midiNoteNumber);
    static float midiToFrequency(float midiNote, float refA4 = 440.0f) noexcept;
    static float frequencyToMidi(float freqHz, float refA4 = 440.0f) noexcept;

private:
    int rootNote { 0 }; // 0 = C
    int scaleType { 0 }; // 0 = Chromatic
    float referenceA4 { 440.0f };

    std::array<bool, 12> customMask;
    std::array<bool, 12> currentScaleMask;

    void updateScaleMask();
};

} // namespace toytune
