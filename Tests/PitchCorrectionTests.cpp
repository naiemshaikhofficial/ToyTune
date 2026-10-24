#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/DSP/PitchShifter.h"
#include "../Source/DSP/CorrectionSmoother.h"
#include <iostream>
#include <cmath>

bool testPitchShifterLatency()
{
    toytune::PitchShifter shifter;
    shifter.prepare(44100.0, 512);

    shifter.setLowLatencyMode(false);
    const int hqLatency = shifter.getLatencySamples();

    shifter.setLowLatencyMode(true);
    const int llLatency = shifter.getLatencySamples();

    const bool passed = (hqLatency > 0 && llLatency > 0 && llLatency < hqLatency);
    std::cout << "  [PitchShifter] HQ Latency: " << hqLatency << " smp | LL Latency: "
              << llLatency << " smp -> " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

bool testCorrectionSmootherModes()
{
    toytune::CorrectionSmoother smoother;
    smoother.prepare(44100.0);

    // HardTune mode with 0ms retune should immediately snap
    const float rawCents = 40.0f;
    const float ratioHard = smoother.processBlock(rawCents, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                  toytune::TuningModeType::HardTune, 256);

    const float expectedRatio = std::pow(2.0f, 40.0f / 1200.0f);
    const bool passedHard = (std::abs(ratioHard - expectedRatio) < 0.005f);

    std::cout << "  [CorrectionSmoother] Hard Tune 40ct snap -> Ratio: " << ratioHard
              << " (Expected: " << expectedRatio << ") -> " << (passedHard ? "PASS" : "FAIL") << "\n";

    // Natural mode should smooth and yield a smaller immediate step
    smoother.reset();
    const float ratioNatural = smoother.processBlock(rawCents, 50.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                     toytune::TuningModeType::Natural, 256);
    const bool passedNatural = (ratioNatural < ratioHard);

    std::cout << "  [CorrectionSmoother] Natural Mode 50ms -> Initial Ratio: " << ratioNatural
              << " (Gradual) -> " << (passedNatural ? "PASS" : "FAIL") << "\n";

    return passedHard && passedNatural;
}

bool testNoteTransitionDecoupling()
{
    toytune::CorrectionSmoother smoother;
    smoother.prepare(44100.0);

    // Initial state: holding note C4 (MIDI 60) with 0 cents error
    smoother.processBlock(0.0f, 60.0f, 95.0f, 80.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                          toytune::TuningModeType::Modern, 128);

    // Note jumps to D4 (MIDI 62) with +50 cents deviation
    // With Note Transition = 80%, the first hop should NOT snap by +50 cents immediately
    // but should glide smoothly and gracefully (ratio near 1.00)
    const float glideRatio = smoother.processBlock(50.0f, 62.0f, 95.0f, 80.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                   toytune::TuningModeType::Modern, 128);

    // With Instant Transition = 0%, the first hop should snap immediately to 50 cents
    smoother.reset();
    smoother.processBlock(0.0f, 60.0f, 95.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                          toytune::TuningModeType::Modern, 128);
    const float instantRatio = smoother.processBlock(50.0f, 62.0f, 95.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                     toytune::TuningModeType::Modern, 128);

    const bool passed = (glideRatio < instantRatio && glideRatio < 1.01f && instantRatio > 1.02f);
    std::cout << "  [NoteTransition] Slew Glide Ratio: " << glideRatio
              << " vs Instant Snap: " << instantRatio << " -> " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

bool testStereoPhasePreservation()
{
    toytune::PitchShifter shifter;
    shifter.prepare(44100.0, 512);
    shifter.setPitchRatio(1.05946f); // +1 semitone

    juce::AudioBuffer<float> buffer(2, 512);
    // Fill identical signal on Left and Right
    for (int i = 0; i < 512; ++i)
    {
        const float s = std::sin(static_cast<float>(i) * 0.1f);
        buffer.setSample(0, i, s);
        buffer.setSample(1, i, s);
    }

    shifter.process(buffer);

    // Left and Right channels must remain identical (phase-locked)
    float maxDiff = 0.0f;
    for (int i = 0; i < 512; ++i)
    {
        const float diff = std::abs(buffer.getSample(0, i) - buffer.getSample(1, i));
        if (diff > maxDiff)
            maxDiff = diff;
    }

    const bool passed = (maxDiff < 1e-6f);
    std::cout << "  [PitchShifter] Stereo Phase Lock Max Delta: " << maxDiff
              << " -> " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

bool runPitchCorrectionTests()
{
    std::cout << "=== Running Pitch Correction Tests ===\n";
    bool allPassed = true;
    allPassed &= testPitchShifterLatency();
    allPassed &= testCorrectionSmootherModes();
    allPassed &= testNoteTransitionDecoupling();
    allPassed &= testStereoPhasePreservation();
    return allPassed;
}
