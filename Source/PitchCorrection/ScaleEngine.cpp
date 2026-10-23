#include "ScaleEngine.h"
#include <algorithm>

namespace toytune
{

ScaleEngine::ScaleEngine()
{
    customMask.fill(true);
    currentScaleMask.fill(true);
    updateScaleMask();
}

void ScaleEngine::setRootNote(int rootIndex)
{
    rootNote = juce::jlimit(0, 11, rootIndex);
    updateScaleMask();
}

void ScaleEngine::setScaleType(int scaleTypeIndex)
{
    scaleType = juce::jlimit(0, 8, scaleTypeIndex);
    updateScaleMask();
}

void ScaleEngine::setCustomNoteActive(int semitoneIndex, bool isActive)
{
    if (semitoneIndex >= 0 && semitoneIndex < 12)
    {
        customMask[semitoneIndex] = isActive;
        if (scaleType == 8) // Custom mode
            updateScaleMask();
    }
}

void ScaleEngine::setCustomMask(const std::array<bool, 12>& mask)
{
    customMask = mask;
    if (scaleType == 8)
        updateScaleMask();
}

void ScaleEngine::setReferencePitch(float refHz)
{
    if (refHz >= 400.0f && refHz <= 480.0f)
        referenceA4 = refHz;
}

bool ScaleEngine::isSemitoneActive(int semitone) const noexcept
{
    const int idx = (semitone % 12 + 12) % 12;
    return currentScaleMask[idx];
}

std::array<bool, 12> ScaleEngine::getActiveMask() const
{
    return currentScaleMask;
}

void ScaleEngine::updateScaleMask()
{
    if (scaleType == 8) // Custom
    {
        currentScaleMask = customMask;
        // Ensure at least one note is active to prevent division by zero
        bool anyActive = false;
        for (bool b : currentScaleMask)
        {
            if (b) { anyActive = true; break; }
        }
        if (!anyActive)
            currentScaleMask[rootNote] = true;
        return;
    }

    currentScaleMask.fill(false);

    // Scale patterns relative to tonic
    // 0: Chromatic
    // 1: Major: 0, 2, 4, 5, 7, 9, 11
    // 2: Natural Minor: 0, 2, 3, 5, 7, 8, 10
    // 3: Harmonic Minor: 0, 2, 3, 5, 7, 8, 11
    // 4: Melodic Minor: 0, 2, 3, 5, 7, 9, 11
    // 5: Pentatonic Major: 0, 2, 4, 7, 9
    // 6: Pentatonic Minor: 0, 3, 5, 7, 10
    // 7: Blues: 0, 3, 5, 6, 7, 10
    std::vector<int> intervals;

    switch (scaleType)
    {
        case 0: // Chromatic
            intervals = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
            break;
        case 1: // Major
            intervals = { 0, 2, 4, 5, 7, 9, 11 };
            break;
        case 2: // Natural Minor
            intervals = { 0, 2, 3, 5, 7, 8, 10 };
            break;
        case 3: // Harmonic Minor
            intervals = { 0, 2, 3, 5, 7, 8, 11 };
            break;
        case 4: // Melodic Minor
            intervals = { 0, 2, 3, 5, 7, 9, 11 };
            break;
        case 5: // Pentatonic Major
            intervals = { 0, 2, 4, 7, 9 };
            break;
        case 6: // Pentatonic Minor
            intervals = { 0, 3, 5, 7, 10 };
            break;
        case 7: // Blues
            intervals = { 0, 3, 5, 6, 7, 10 };
            break;
        default:
            intervals = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
            break;
    }

    for (int interval : intervals)
    {
        const int pitchClass = (rootNote + interval) % 12;
        currentScaleMask[pitchClass] = customMask[pitchClass];
    }

    // Ensure at least one note remains active
    bool anyActive = false;
    for (bool b : currentScaleMask)
    {
        if (b) { anyActive = true; break; }
    }
    if (!anyActive)
        currentScaleMask[rootNote] = true;
}

TargetPitchResult ScaleEngine::calculateTarget(float inputMidiNote) const
{
    TargetPitchResult result;
    if (inputMidiNote <= 12.0f || inputMidiNote >= 127.0f)
        return result;

    const int roundedInput = static_cast<int>(std::round(inputMidiNote));
    float closestMidi = static_cast<float>(roundedInput);
    float minDistance = 999.0f;

    // Search outward within +/- 6 semitones for the nearest active scale note
    for (int offset = -6; offset <= 6; ++offset)
    {
        const int candidateMidi = roundedInput + offset;
        if (candidateMidi < 12 || candidateMidi > 127)
            continue;

        const int pitchClass = (candidateMidi % 12 + 12) % 12;
        if (currentScaleMask[pitchClass])
        {
            const float dist = std::abs(static_cast<float>(candidateMidi) - inputMidiNote);
            if (dist < minDistance)
            {
                minDistance = dist;
                closestMidi = static_cast<float>(candidateMidi);
            }
        }
    }

    result.targetMidiNote = closestMidi;
    result.targetFreqHz = midiToFrequency(closestMidi, referenceA4);
    result.centsDeviation = (closestMidi - inputMidiNote) * 100.0f;

    const int targetInt = static_cast<int>(std::round(closestMidi));
    result.targetSemitone = (targetInt % 12 + 12) % 12;
    result.targetOctave = (targetInt / 12) - 1;
    result.noteName = getNoteNameWithOctave(targetInt);

    return result;
}

juce::String ScaleEngine::getNoteNameWithOctave(int midiNoteNumber)
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int note = (midiNoteNumber % 12 + 12) % 12;
    const int octave = (midiNoteNumber / 12) - 1;
    return juce::String(noteNames[note]) + juce::String(octave);
}

float ScaleEngine::midiToFrequency(float midiNote, float refA4) noexcept
{
    return refA4 * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

float ScaleEngine::frequencyToMidi(float freqHz, float refA4) noexcept
{
    if (freqHz <= 1.0f)
        return 0.0f;
    return 69.0f + 12.0f * std::log2(freqHz / refA4);
}

} // namespace toytune
