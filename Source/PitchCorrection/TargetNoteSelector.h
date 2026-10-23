#pragma once

#include "ScaleEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace toytune
{

class TargetNoteSelector
{
public:
    TargetNoteSelector() = default;

    void processMidi(const juce::MidiBuffer& midiMessages)
    {
        for (const auto metadata : midiMessages)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                lastMidiNoteOn.store(msg.getNoteNumber(), std::memory_order_relaxed);
                hasActiveMidiNote.store(true, std::memory_order_relaxed);
            }
            else if (msg.isNoteOff())
            {
                if (lastMidiNoteOn.load(std::memory_order_relaxed) == msg.getNoteNumber())
                {
                    hasActiveMidiNote.store(false, std::memory_order_relaxed);
                }
            }
            else if (msg.isAllNotesOff())
            {
                hasActiveMidiNote.store(false, std::memory_order_relaxed);
            }
        }
    }

    TargetPitchResult resolveTarget(const ScaleEngine& scaleEngine, float inputMidiNote) const
    {
        if (hasActiveMidiNote.load(std::memory_order_relaxed))
        {
            const int targetNote = lastMidiNoteOn.load(std::memory_order_relaxed);
            TargetPitchResult res;
            res.targetMidiNote = static_cast<float>(targetNote);
            res.targetFreqHz = ScaleEngine::midiToFrequency(res.targetMidiNote, scaleEngine.getReferencePitch());
            res.centsDeviation = (res.targetMidiNote - inputMidiNote) * 100.0f;
            res.targetSemitone = (targetNote % 12 + 12) % 12;
            res.targetOctave = (targetNote / 12) - 1;
            res.noteName = ScaleEngine::getNoteNameWithOctave(targetNote);
            return res;
        }

        return scaleEngine.calculateTarget(inputMidiNote);
    }

    void reset()
    {
        hasActiveMidiNote.store(false, std::memory_order_relaxed);
        lastMidiNoteOn.store(60, std::memory_order_relaxed);
    }

private:
    std::atomic<int> lastMidiNoteOn { 60 };
    std::atomic<bool> hasActiveMidiNote { false };
};

} // namespace toytune
