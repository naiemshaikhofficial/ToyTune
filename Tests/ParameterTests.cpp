#include <juce_core/juce_core.h>
#include "../Source/Parameters/ToyTuneParameters.h"
#include <iostream>

// Declarations of external test runners
bool runPitchDetectionTests();
bool runScaleEngineTests();
bool runPitchCorrectionTests();

bool testParameterLayout()
{
    std::cout << "=== Running Parameter Tests ===\n";
    auto layout = toytune::ToyTuneParameters::createParameterLayout();

    // Verify key parameters exist
    const auto& rootNames = toytune::ToyTuneParameters::getRootNoteNames();
    const auto& scaleNames = toytune::ToyTuneParameters::getScaleTypeNames();
    const auto& modeNames = toytune::ToyTuneParameters::getTuningModeNames();

    const bool passed = (rootNames.size() == 12 &&
                         scaleNames.size() >= 8 &&
                         modeNames.size() == 4);

    std::cout << "  [Parameters] Root keys: " << rootNames.size()
              << " | Scales: " << scaleNames.size()
              << " | Modes: " << modeNames.size()
              << " -> " << (passed ? "PASS" : "FAIL") << "\n";

    return passed;
}

int main(int /*argc*/, char* /*argv*/[])
{
    std::cout << "\n========================================\n";
    std::cout << "  TOYTUNE DSP & AUDIO ENGINE TEST SUITE \n";
    std::cout << "========================================\n\n";

    bool success = true;
    success &= runPitchDetectionTests();
    std::cout << "\n";
    success &= runScaleEngineTests();
    std::cout << "\n";
    success &= runPitchCorrectionTests();
    std::cout << "\n";
    success &= testParameterLayout();

    std::cout << "\n========================================\n";
    if (success)
    {
        std::cout << "  ALL TOYTUNE DSP TESTS PASSED (100%)\n";
        std::cout << "========================================\n\n";
        return 0;
    }
    else
    {
        std::cout << "  ONE OR MORE TESTS FAILED!\n";
        std::cout << "========================================\n\n";
        return 1;
    }
}
