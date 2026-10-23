#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/PitchDetection/PitchDetector.h"
#include <iostream>
#include <cmath>

bool testPitchAtFrequency(float testFreqHz, double sampleRate, int blockSize)
{
    toytune::PitchDetector detector;
    detector.prepare(sampleRate, blockSize);

    std::vector<float> buffer(blockSize);
    double phase = 0.0;
    const double phaseInc = (juce::MathConstants<double>::twoPi * testFreqHz) / sampleRate;

    toytune::PitchDetectionResult result;
    // Feed several blocks to warm up analysis buffer
    for (int block = 0; block < 10; ++block)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            buffer[i] = static_cast<float>(std::sin(phase));
            phase += phaseInc;
            if (phase >= juce::MathConstants<double>::twoPi)
                phase -= juce::MathConstants<double>::twoPi;
        }
        result = detector.processBlock(buffer.data(), blockSize);
    }

    const float errorHz = std::abs(result.pitchHz - testFreqHz);
    const float errorPercent = (errorHz / testFreqHz) * 100.0f;
    const bool passed = (result.isVoiced && errorPercent < 3.0f);

    std::cout << "  [PitchDetector] Freq: " << testFreqHz << " Hz | Detected: " << result.pitchHz
              << " Hz | Conf: " << result.confidence << " | Error: " << errorPercent << "% -> "
              << (passed ? "PASS" : "FAIL") << "\n";

    return passed;
}

bool testSilenceRejection()
{
    toytune::PitchDetector detector;
    detector.prepare(44100.0, 512);

    std::vector<float> silenceBuffer(512, 0.0f);
    auto result = detector.processBlock(silenceBuffer.data(), 512);

    const bool passed = (!result.isVoiced);
    std::cout << "  [PitchDetector] Silence Gating: isVoiced = " << (result.isVoiced ? "true" : "false")
              << " -> " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

bool runPitchDetectionTests()
{
    std::cout << "=== Running Pitch Detection Tests ===\n";
    bool allPassed = true;
    allPassed &= testPitchAtFrequency(110.0f, 44100.0, 512); // A2
    allPassed &= testPitchAtFrequency(220.0f, 44100.0, 512); // A3
    allPassed &= testPitchAtFrequency(440.0f, 44100.0, 512); // A4
    allPassed &= testPitchAtFrequency(880.0f, 44100.0, 512); // A5
    allPassed &= testSilenceRejection();
    return allPassed;
}
