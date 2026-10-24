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

    TargetPitchResult resolveTarget(const ScaleEngine& scaleEngine, float inputMidiNote)
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
            lastTargetNote = res;
            hasValidTarget = true;
            return res;
        }

        // Hysteresis Note Latching (Waves Tune Real-Time Anti-Chatter):
        // If we already have an active target note in scale and the vocal pitch is within +/- 58 cents,
        // stay locked to it to prevent rapid ping-pong jitter on borderline microtones.
        if (hasValidTarget && !lastTargetNote.isBypassed && scaleEngine.isSemitoneActive(lastTargetNote.targetSemitone))
        {
            const float deviationFromCurrent = (lastTargetNote.targetMidiNote - inputMidiNote) * 100.0f;
            if (std::abs(deviationFromCurrent) < 58.0f)
            {
                TargetPitchResult res = lastTargetNote;
                res.centsDeviation = deviationFromCurrent;
                return res;
            }
        }

        const auto newTarget = scaleEngine.calculateTarget(inputMidiNote);
        lastTargetNote = newTarget;
        hasValidTarget = !newTarget.isBypassed;
        return newTarget;
    }

    void reset()
    {
        hasActiveMidiNote.store(false, std::memory_order_relaxed);
        lastMidiNoteOn.store(60, std::memory_order_relaxed);
        hasValidTarget = false;
    }

private:
    std::atomic<int> lastMidiNoteOn { 60 };
    std::atomic<bool> hasActiveMidiNote { false };
    TargetPitchResult lastTargetNote;
    bool hasValidTarget { false };
};

} // namespace toytune
