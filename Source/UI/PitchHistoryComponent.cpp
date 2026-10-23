#include "PitchHistoryComponent.h"
#include <cmath>

namespace toytune
{

PitchHistoryComponent::PitchHistoryComponent(AudioTelemetry& telemetry)
    : audioTelemetry(telemetry)
{
    detectedPoints.resize(AudioTelemetry::kHistoryCapacity, 0.0f);
    targetPoints.resize(AudioTelemetry::kHistoryCapacity, 0.0f);
    setOpaque(true);
}

void PitchHistoryComponent::updateTelemetry()
{
    const int writePos = audioTelemetry.historyWritePos.load(std::memory_order_acquire);
    for (int i = 0; i < AudioTelemetry::kHistoryCapacity; ++i)
    {
        const int readIdx = (writePos + i) % AudioTelemetry::kHistoryCapacity;
        const float detHz = audioTelemetry.pitchHistory[readIdx];
        const float tgtHz = audioTelemetry.targetHistory[readIdx];

        detectedPoints[i] = (detHz > 20.0f) ? (69.0f + 12.0f * std::log2(detHz / 440.0f)) : 0.0f;
        targetPoints[i]   = (tgtHz > 20.0f) ? (69.0f + 12.0f * std::log2(tgtHz / 440.0f)) : 0.0f;
    }
    repaint();
}

float PitchHistoryComponent::midiToY(float midiNote, float height) const noexcept
{
    const float norm = (midiNote - minMidi) / (maxMidi - minMidi);
    return height - (juce::jlimit(0.0f, 1.0f, norm) * height);
}

void PitchHistoryComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Dark recessed background
    g.setColour(juce::Colour(0xff0b0b0d));
    g.fillRect(bounds);

    // Subtle horizontal pitch grid lines (Semitones & Octaves)
    for (int midi = static_cast<int>(minMidi); midi <= static_cast<int>(maxMidi); ++midi)
    {
        const float y = midiToY(static_cast<float>(midi), h);
        const bool isOctaveC = ((midi % 12) == 0);

        if (isOctaveC)
        {
            g.setColour(juce::Colour(0x35ffffff));
            g.drawHorizontalLine(static_cast<int>(y), 0.0f, w);

            // Octave label
            const int oct = (midi / 12) - 1;
            g.setFont(juce::Font(9.0f, juce::Font::bold));
            g.drawText("C" + juce::String(oct), 4, static_cast<int>(y - 10.0f), 24, 10, juce::Justification::centredLeft);
        }
        else
        {
            g.setColour(juce::Colour(0x12ffffff));
            g.drawHorizontalLine(static_cast<int>(y), 0.0f, w);
        }
    }

    // Draw Target Corrected Pitch Lines (Subtle Silver)
    juce::Path targetPath;
    bool hasTargetSegment = false;

    const int numPoints = static_cast<int>(targetPoints.size());
    for (int i = 0; i < numPoints; ++i)
    {
        const float targetMidi = targetPoints[i];
        const float x = (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * w;

        if (targetMidi > 10.0f)
        {
            const float y = midiToY(targetMidi, h);
            if (!hasTargetSegment)
            {
                targetPath.startNewSubPath(x, y);
                hasTargetSegment = true;
            }
            else
            {
                targetPath.lineTo(x, y);
            }
        }
        else
        {
            hasTargetSegment = false;
        }
    }

    g.setColour(juce::Colour(0x60c0c0c8));
    g.strokePath(targetPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Draw Detected Pitch Trace (Bright Pearl White with soft glow)
    juce::Path detectedPath;
    bool hasDetectSegment = false;

    for (int i = 0; i < numPoints; ++i)
    {
        const float detMidi = detectedPoints[i];
        const float x = (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * w;

        if (detMidi > 10.0f)
        {
            const float y = midiToY(detMidi, h);
            if (!hasDetectSegment)
            {
                detectedPath.startNewSubPath(x, y);
                hasDetectSegment = true;
            }
            else
            {
                detectedPath.lineTo(x, y);
            }
        }
        else
        {
            hasDetectSegment = false;
        }
    }

    // Subtle glow
    g.setColour(juce::Colour(0x20ffffff));
    g.strokePath(detectedPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Core trace line
    g.setColour(juce::Colour(0xfff0f0f4));
    g.strokePath(detectedPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Outer recessed panel border
    g.setColour(juce::Colour(0xff1e1e24));
    g.drawRect(bounds, 1.0f);
}

} // namespace toytune
