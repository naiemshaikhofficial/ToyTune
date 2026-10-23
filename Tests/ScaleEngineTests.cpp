#include <juce_core/juce_core.h>
#include "../Source/PitchCorrection/ScaleEngine.h"
#include <iostream>
#include <cmath>

bool testChromaticScale()
{
    toytune::ScaleEngine scale;
    scale.setRootNote(0); // C
    scale.setScaleType(0); // Chromatic

    // Input: C#4 slightly sharp (61.35)
    auto target = scale.calculateTarget(61.35f);
    const bool passed = (std::abs(target.targetMidiNote - 61.0f) < 0.01f &&
                         std::abs(target.centsDeviation - (-35.0f)) < 0.5f);

    std::cout << "  [ScaleEngine] Chromatic 61.35 -> Target: " << target.targetMidiNote
              << " (" << target.noteName.toStdString() << ") Dev: " << target.centsDeviation
              << " ct -> " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

bool testMajorScale()
{
    toytune::ScaleEngine scale;
    scale.setRootNote(0); // C Major: C, D, E, F, G, A, B (no C# / 61)
    scale.setScaleType(1); // Major

    // Input: C#4 (61.0f) -> Nearest in C Major is D (62) or C (60)
    auto target = scale.calculateTarget(61.1f);
    const bool passed = (std::abs(target.targetMidiNote - 62.0f) < 0.01f ||
                         std::abs(target.targetMidiNote - 60.0f) < 0.01f);

    std::cout << "  [ScaleEngine] C Major on C# (61.1) -> Target: " << target.targetMidiNote
              << " (" << target.noteName.toStdString() << ") -> " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

bool testCustomNoteMask()
{
    toytune::ScaleEngine scale;
    scale.setScaleType(8); // Custom
    // Disable all notes except A (semitone 9)
    for (int i = 0; i < 12; ++i)
        scale.setCustomNoteActive(i, (i == 9));

    // Input: C4 (60.0) -> Nearest active note must be A3 (57) or A4 (69)
    auto target = scale.calculateTarget(60.0f);
    const bool passed = (target.targetSemitone == 9);

    std::cout << "  [ScaleEngine] Custom Mask Only A -> Target Note: " << target.targetMidiNote
              << " (" << target.noteName.toStdString() << ") -> " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

bool runScaleEngineTests()
{
    std::cout << "=== Running Scale Engine Tests ===\n";
    bool allPassed = true;
    allPassed &= testChromaticScale();
    allPassed &= testMajorScale();
    allPassed &= testCustomNoteMask();
    return allPassed;
}
