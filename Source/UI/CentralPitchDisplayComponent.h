#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/AudioTelemetry.h"
#include "../PitchCorrection/ScaleEngine.h"
#include <cmath>
#include <array>

namespace toytune
{

/**
 * Authentic Circular Pitch Display matching Producer Toy ToyTune reference image.
 * Features:
 * - Ambient flanking audio waveform graphic on left and right
 * - Outer metallic ring with radial calibration tick marks
 * - Orbiting glowing pitch deviation pointer bead
 * - Glossy black convex dome with glass highlight reflection
 * - Massive center musical note display (e.g. "B") and "PITCH" sub-label
 * - Horizontal Pitch Deviation ruler (-100, -50, 0, +50, +100) with animated needle
 */
class CentralPitchDisplayComponent : public juce::Component
{
public:
    explicit CentralPitchDisplayComponent(AudioTelemetry& telemetry)
        : audioTelemetry(telemetry)
    {
        for (size_t i = 0; i < waveformBarsL.size(); ++i)
        {
            waveformBarsL[i] = 0.05f;
            waveformBarsR[i] = 0.05f;
        }
    }

    void updateTelemetry()
    {
        const float targetDev = audioTelemetry.centsDeviation.load(std::memory_order_relaxed);
        const bool voiced = audioTelemetry.isVoiced.load(std::memory_order_relaxed);
        const float inRms = audioTelemetry.inputRmsDb.load(std::memory_order_relaxed);

        // Smooth deviation pointer
        if (voiced)
        {
            smoothedCents += 0.25f * (targetDev - smoothedCents);
            idleAlpha = std::min(1.0f, idleAlpha + 0.1f);
        }
        else
        {
            smoothedCents += 0.1f * (0.0f - smoothedCents);
            idleAlpha = std::max(0.2f, idleAlpha - 0.04f);
        }

        // Update ambient waveform bars based on real-time vocal RMS
        const float normRms = juce::jlimit(0.0f, 1.0f, (inRms + 60.0f) / 60.0f);
        wavePhase += 0.08f;

        for (size_t i = 0; i < waveformBarsL.size(); ++i)
        {
            const float idxNorm = static_cast<float>(i) / static_cast<float>(waveformBarsL.size() - 1);
            const float envelope = std::sin(idxNorm * juce::MathConstants<float>::pi);
            const float modulation = 0.5f + 0.5f * std::sin(wavePhase + static_cast<float>(i) * 0.45f);
            const float targetHeight = normRms * envelope * (0.3f + 0.7f * modulation);

            waveformBarsL[i] = 0.8f * waveformBarsL[i] + 0.2f * targetHeight;
            waveformBarsR[i] = 0.8f * waveformBarsR[i] + 0.2f * targetHeight;
        }

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        if (bounds.isEmpty())
            return;

        const float centreX = bounds.getCentreX();
        const float circleCentreY = bounds.getY() + bounds.getHeight() * 0.44f;
        const float circleDiameter = juce::jmin(bounds.getWidth() * 0.38f, bounds.getHeight() * 0.68f);
        const float circleRadius = circleDiameter * 0.5f;

        // 1. Draw Flanking Ambient Waveforms (Left and Right)
        drawFlankingWaveforms(g, centreX, circleCentreY, circleRadius, bounds);

        // 2. Draw Central Circular Pitch Sphere
        drawCircularPitchMeter(g, centreX, circleCentreY, circleRadius);

        // 3. Draw Pitch Deviation Ruler below Circle
        drawDeviationRuler(g, centreX, circleCentreY + circleRadius + 14.0f, bounds.getWidth() * 0.72f);
    }

private:
    AudioTelemetry& audioTelemetry;

    float smoothedCents { 0.0f };
    float idleAlpha { 0.2f };
    float wavePhase { 0.0f };

    static constexpr int kNumWaveBars = 18;
    std::array<float, kNumWaveBars> waveformBarsL;
    std::array<float, kNumWaveBars> waveformBarsR;

    void drawFlankingWaveforms(juce::Graphics& g, float cx, float cy, float radius, const juce::Rectangle<float>& bounds)
    {
        const float spacing = 6.5f;
        const float barW = 2.5f;
        const float maxBarH = radius * 0.85f;

        // Left Waveform
        const float startXL = cx - radius - 16.0f;
        for (int i = 0; i < kNumWaveBars; ++i)
        {
            const float x = startXL - static_cast<float>(i) * spacing;
            if (x < bounds.getX() + 8.0f)
                break;

            const float h = std::max(3.0f, waveformBarsL[static_cast<size_t>(i)] * maxBarH);
            const float y = cy - h * 0.5f;

            const float barAlpha = juce::jlimit(0.08f, 0.45f, waveformBarsL[static_cast<size_t>(i)] * 0.7f + 0.08f);
            g.setColour(juce::Colour(0xff8a8d9a).withAlpha(barAlpha));
            g.fillRoundedRectangle(x, y, barW, h, 1.2f);
        }

        // Right Waveform
        const float startXR = cx + radius + 16.0f;
        for (int i = 0; i < kNumWaveBars; ++i)
        {
            const float x = startXR + static_cast<float>(i) * spacing;
            if (x > bounds.getRight() - 8.0f)
                break;

            const float h = std::max(3.0f, waveformBarsR[static_cast<size_t>(i)] * maxBarH);
            const float y = cy - h * 0.5f;

            const float barAlpha = juce::jlimit(0.08f, 0.45f, waveformBarsR[static_cast<size_t>(i)] * 0.7f + 0.08f);
            g.setColour(juce::Colour(0xff8a8d9a).withAlpha(barAlpha));
            g.fillRoundedRectangle(x, y, barW, h, 1.2f);
        }
    }

    void drawCircularPitchMeter(juce::Graphics& g, float cx, float cy, float radius)
    {
        // Outer Shadow for tactile depth
        g.setColour(juce::Colour(0x80000000));
        g.fillEllipse(cx - radius - 3.0f, cy - radius - 1.0f, (radius + 3.0f) * 2.0f, (radius + 5.0f) * 2.0f);

        // 1. Outer Dark Metallic Bezel
        juce::ColourGradient bezelGrad(juce::Colour(0xff2d2e35), cx, cy - radius,
                                       juce::Colour(0xff121316), cx, cy + radius, false);
        g.setGradientFill(bezelGrad);
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colour(0xff3a3c44));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.2f);

        // 2. Radial Calibration Ticks in Bezel Ring
        const float innerRingRadius = radius - 10.0f;
        const int numTicks = 48;
        for (int i = 0; i < numTicks; ++i)
        {
            const float angle = static_cast<float>(i) * (juce::MathConstants<float>::twoPi / static_cast<float>(numTicks));
            const bool isMajor = (i % 4 == 0);
            const float tickInner = isMajor ? (innerRingRadius + 1.0f) : (innerRingRadius + 4.0f);
            const float tickOuter = radius - 3.0f;

            const float cosA = std::cos(angle);
            const float sinA = std::sin(angle);

            g.setColour(isMajor ? juce::Colour(0x60ffffff) : juce::Colour(0x25ffffff));
            g.drawLine(cx + cosA * tickInner, cy + sinA * tickInner,
                       cx + cosA * tickOuter, cy + sinA * tickOuter,
                       isMajor ? 1.4f : 0.8f);
        }

        // 3. Orbiting Glowing Pitch Deviation Bead & Arc
        // Maps -50..+50 cents deviation to -120 deg .. +120 deg around top
        const float deviationNorm = juce::jlimit(-1.0f, 1.0f, smoothedCents / 50.0f);
        // Top center is -pi/2
        const float pointerAngle = -juce::MathConstants<float>::halfPi + deviationNorm * (juce::MathConstants<float>::pi * 0.65f);

        // Arc track background
        juce::Path arcTrack;
        arcTrack.addCentredArc(cx, cy, innerRingRadius + 4.0f, innerRingRadius + 4.0f, 0.0f,
                               -juce::MathConstants<float>::halfPi - juce::MathConstants<float>::pi * 0.65f,
                               -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::pi * 0.65f, true);
        g.setColour(juce::Colour(0x20ffffff));
        g.strokePath(arcTrack, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active deviation arc
        juce::Path activeArc;
        activeArc.addCentredArc(cx, cy, innerRingRadius + 4.0f, innerRingRadius + 4.0f, 0.0f,
                                -juce::MathConstants<float>::halfPi, pointerAngle, true);
        g.setColour(juce::Colour(0xd0ffffff));
        g.strokePath(activeArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Glowing Pointer Bead
        const float beadRadius = 5.5f;
        const float beadX = cx + std::cos(pointerAngle) * (innerRingRadius + 4.0f);
        const float beadY = cy + std::sin(pointerAngle) * (innerRingRadius + 4.0f);

        // Bead glow
        g.setColour(juce::Colour(0x50ffffff));
        g.fillEllipse(beadX - beadRadius - 2.5f, beadY - beadRadius - 2.5f, (beadRadius + 2.5f) * 2.0f, (beadRadius + 2.5f) * 2.0f);
        // Solid white bead
        g.setColour(juce::Colour(0xffffffff));
        g.fillEllipse(beadX - beadRadius, beadY - beadRadius, beadRadius * 2.0f, beadRadius * 2.0f);

        // 4. Inner Glossy Convex Dome
        const float domeRadius = innerRingRadius - 2.0f;
        juce::ColourGradient domeGrad(juce::Colour(0xff18191d), cx, cy - domeRadius * 0.6f,
                                      juce::Colour(0xff09090b), cx, cy + domeRadius, false);
        g.setGradientFill(domeGrad);
        g.fillEllipse(cx - domeRadius, cy - domeRadius, domeRadius * 2.0f, domeRadius * 2.0f);

        // Subtle 3D inner rim shadow
        g.setColour(juce::Colour(0x50000000));
        g.drawEllipse(cx - domeRadius, cy - domeRadius, domeRadius * 2.0f, domeRadius * 2.0f, 2.0f);

        // Glass curved highlight reflection (upper hemisphere crescent)
        juce::Path glassHighlight;
        glassHighlight.addEllipse(cx - domeRadius * 0.85f, cy - domeRadius * 0.92f, domeRadius * 1.7f, domeRadius * 0.95f);
        juce::ColourGradient glassGrad(juce::Colour(0x35ffffff), cx, cy - domeRadius * 0.9f,
                                       juce::Colour(0x00ffffff), cx, cy - domeRadius * 0.1f, false);
        g.setGradientFill(glassGrad);
        g.fillPath(glassHighlight);

        // 5. Note Text & "PITCH" Subtitle
        const bool voiced = audioTelemetry.isVoiced.load(std::memory_order_relaxed);
        const float detMidi = audioTelemetry.detectedMidiNote.load(std::memory_order_relaxed);
        const float targetMidi = audioTelemetry.targetMidiNote.load(std::memory_order_relaxed);

        juce::String noteName = "--";
        if (voiced && detMidi >= 20.0f)
        {
            const int noteIdx = (static_cast<int>(std::round(targetMidi > 0.0f ? targetMidi : detMidi)) % 12 + 12) % 12;
            static const char* kNotes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            noteName = kNotes[noteIdx];
        }

        // Draw bold note name
        g.setFont(juce::Font(juce::FontOptions(domeRadius * 0.68f).withStyle("Bold")));
        g.setColour(voiced ? juce::Colour(0xffffffff) : juce::Colour(0xff757885));
        g.drawText(noteName,
                   static_cast<int>(cx - domeRadius),
                   static_cast<int>(cy - domeRadius * 0.45f),
                   static_cast<int>(domeRadius * 2.0f),
                   static_cast<int>(domeRadius * 0.75f),
                   juce::Justification::centred, false);

        // Draw "PITCH" label
        g.setFont(juce::Font(juce::FontOptions(domeRadius * 0.16f).withStyle("Bold")));
        g.setColour(juce::Colour(0xff7a7d88));
        g.drawText("PITCH",
                   static_cast<int>(cx - domeRadius),
                   static_cast<int>(cy + domeRadius * 0.32f),
                   static_cast<int>(domeRadius * 2.0f),
                   static_cast<int>(domeRadius * 0.25f),
                   juce::Justification::centred, false);
    }

    void drawDeviationRuler(juce::Graphics& g, float cx, float rulerY, float rulerWidth)
    {
        const float halfW = rulerWidth * 0.5f;
        const float leftX = cx - halfW;
        const float rightX = cx + halfW;

        // Baseline tick line
        g.setColour(juce::Colour(0x35ffffff));
        g.drawHorizontalLine(static_cast<int>(rulerY), leftX, rightX);

        // Major markings: -100, -50, 0, +50, +100
        struct RulerMark { float cents; const char* label; };
        static const RulerMark kMarks[] = {
            { -100.0f, "-100" },
            {  -50.0f,  "-50" },
            {    0.0f,    "0" },
            {  +50.0f,  "+50" },
            { +100.0f, "+100" }
        };

        g.setFont(juce::Font(juce::FontOptions(10.0f)));

        for (const auto& mark : kMarks)
        {
            const float norm = (mark.cents + 100.0f) / 200.0f;
            const float x = leftX + norm * rulerWidth;

            // Major tick mark
            g.setColour(juce::Colour(0x60ffffff));
            g.drawVerticalLine(static_cast<int>(x), rulerY - 4.0f, rulerY + 5.0f);

            // Text label below tick
            g.setColour(juce::Colour(0xff8a8d9a));
            g.drawText(mark.label,
                       static_cast<int>(x - 22.0f),
                       static_cast<int>(rulerY + 6.0f),
                       44, 14, juce::Justification::centred, false);
        }

        // Minor ticks every 10 cents
        for (int c = -90; c <= 90; c += 10)
        {
            if (c % 50 == 0) continue;
            const float norm = static_cast<float>(c + 100) / 200.0f;
            const float x = leftX + norm * rulerWidth;
            g.setColour(juce::Colour(0x20ffffff));
            g.drawVerticalLine(static_cast<int>(x), rulerY - 2.0f, rulerY + 3.0f);
        }

        // Active needle indicating current real-time deviation
        const bool voiced = audioTelemetry.isVoiced.load(std::memory_order_relaxed);
        if (voiced)
        {
            const float needleNorm = juce::jlimit(0.0f, 1.0f, (smoothedCents + 100.0f) / 200.0f);
            const float needleX = leftX + needleNorm * rulerWidth;

            // Needle pip
            g.setColour(juce::Colour(0xffffffff));
            g.fillEllipse(needleX - 2.5f, rulerY - 6.0f, 5.0f, 5.0f);
            g.drawVerticalLine(static_cast<int>(needleX), rulerY - 6.0f, rulerY + 5.0f);
        }
    }
};

} // namespace toytune
