#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BinaryData.h"
#include "../Presets/PresetManager.h"
#include "../Parameters/ToyTuneParameters.h"

namespace toytune
{

/**
 * Top Header Bar matching Producer Toy ToyTune reference image.
 * Features:
 * - Left: Producer Toy 3D Logo + Divider + ToyTune Title + "REAL-TIME VOCAL TUNE"
 * - Center: Preset Bar (<, Preset Name, Subtitle, >, Save, Delete, Settings)
 * - Right: Producer Toy Mascot Bear with headphones + 3 angled grip lines
 */
class HeaderBarComponent : public juce::Component
{
public:
    HeaderBarComponent(juce::AudioProcessorValueTreeState& apvts, PresetManager& pManager)
        : presetManager(pManager)
    {
        juce::ignoreUnused(apvts);
        // Load embedded logos and mascot
        logoImage = juce::ImageCache::getFromMemory(BinaryData::producertoycharcoallogo_png,
                                                   BinaryData::producertoycharcoallogo_pngSize);
        wordmarkImage = juce::ImageCache::getFromMemory(BinaryData::toytunewordmark_png,
                                                       BinaryData::toytunewordmark_pngSize);
        mascotImage = juce::ImageCache::getFromMemory(BinaryData::managerbearmascot_png,
                                                     BinaryData::managerbearmascot_pngSize);

        // Previous Preset Button (<)
        prevBtn.setButtonText("<");
        prevBtn.onClick = [this]()
        {
            const int count = presetManager.getPresetNames().size();
            if (count > 0)
            {
                int current = presetManager.getCurrentPresetIndex();
                int next = (current - 1 + count) % count;
                presetManager.applyPreset(next);
                updatePresetDisplay();
            }
        };
        addAndMakeVisible(prevBtn);

        // Next Preset Button (>)
        nextBtn.setButtonText(">");
        nextBtn.onClick = [this]()
        {
            const int count = presetManager.getPresetNames().size();
            if (count > 0)
            {
                int current = presetManager.getCurrentPresetIndex();
                int next = (current + 1) % count;
                presetManager.applyPreset(next);
                updatePresetDisplay();
            }
        };
        addAndMakeVisible(nextBtn);

        // Save Preset Button (Floppy Disk icon)
        saveBtn.setButtonText("");
        saveBtn.onClick = [this]()
        {
            auto* parent = getTopLevelComponent();
            juce::AlertWindow::showOkCancelBox(
                juce::AlertWindow::QuestionIcon,
                "Save Vocal Preset",
                "Enter preset name:",
                "Save", "Cancel",
                parent,
                juce::ModalCallbackFunction::create([this](int result)
                {
                    if (result != 0)
                    {
                        presetManager.saveUserPreset("Custom Vocal " + juce::String(juce::Random::getSystemRandom().nextInt(99)));
                        updatePresetDisplay();
                    }
                }));
        };
        addAndMakeVisible(saveBtn);

        // Delete Preset Button (Trash Can icon)
        deleteBtn.setButtonText("");
        deleteBtn.onClick = [this]()
        {
            // Reset to Default preset
            presetManager.applyPreset(0);
            updatePresetDisplay();
        };
        addAndMakeVisible(deleteBtn);

        // Settings Button (Gear icon)
        settingsBtn.setButtonText("");
        settingsBtn.onClick = [this]()
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "ToyTune Settings",
                "ToyTune by Producer Toy\nVersion 1.0.0 (64-bit)\n\n"
                "• Engine: Single-Trajectory Period-Synchronous Resampling\n"
                "• Pitch Detection: CMNDF / YIN with Parabolic Peak Interpolation\n"
                "• Formant Tracking: Pure In-Phase Transposition\n"
                "• Latency: High-Quality (1024 smp) / Low-Latency (512 smp)\n\n"
                "Producer Toy — Luxury Audio DSP");
        };
        addAndMakeVisible(settingsBtn);

        updatePresetDisplay();
    }

    void updatePresetDisplay()
    {
        const int idx = presetManager.getCurrentPresetIndex();
        const auto names = presetManager.getPresetNames();
        if (idx >= 0 && idx < names.size())
            currentPresetName = names[idx];
        else
            currentPresetName = "Default";

        isFactoryPreset = (idx < static_cast<int>(presetManager.getFactoryPresets().size()));
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // 1. Left Branding: PRODUCER TOY Logo + Divider + ToyTune Title
        float curX = bounds.getX() + 18.0f;
        const float logoSize = 38.0f;
        const float logoY = bounds.getY() + (bounds.getHeight() - logoSize) * 0.5f;

        if (logoImage.isValid())
        {
            g.drawImage(logoImage, static_cast<int>(curX), static_cast<int>(logoY),
                        static_cast<int>(logoSize), static_cast<int>(logoSize),
                        0, 0, logoImage.getWidth(), logoImage.getHeight());
            curX += logoSize + 12.0f;
        }

        // Vertical metallic hairline divider
        g.setColour(juce::Colour(0x35ffffff));
        g.drawVerticalLine(static_cast<int>(curX), bounds.getY() + 12.0f, bounds.getBottom() - 12.0f);
        curX += 14.0f;

        // "ToyTune" Title and "REAL-TIME VOCAL TUNE"
        const float textY = bounds.getY() + 10.0f;
        g.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
        g.setColour(juce::Colour(0xffffffff));
        g.drawText("Toy", static_cast<int>(curX), static_cast<int>(textY), 40, 24, juce::Justification::centredLeft, false);

        juce::ColourGradient tuneGrad(juce::Colour(0xffffffff), curX + 38.0f, textY,
                                      juce::Colour(0xffb5b8c5), curX + 38.0f, textY + 24.0f, false);
        g.setGradientFill(tuneGrad);
        g.drawText("Tune", static_cast<int>(curX + 38.0f), static_cast<int>(textY), 65, 24, juce::Justification::centredLeft, false);

        // Subtitle: REAL-TIME VOCAL TUNE
        g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));
        g.setColour(juce::Colour(0xff828590));
        g.drawText("REAL-TIME  VOCAL  TUNE", static_cast<int>(curX + 1.0f), static_cast<int>(textY + 24.0f), 150, 12, juce::Justification::centredLeft, false);

        // 2. Center Preset Bar Container
        const auto presetBarRect = presetBarBounds.toFloat();
        g.setColour(juce::Colour(0xff121316));
        g.fillRoundedRectangle(presetBarRect, 8.0f);
        g.setColour(juce::Colour(0xff23252c));
        g.drawRoundedRectangle(presetBarRect, 8.0f, 1.2f);

        // Preset Name & Subtitle in Center
        auto textArea = presetBarRect.reduced(36.0f, 3.0f);
        const auto topArea = textArea.removeFromTop(16.0f);
        g.setFont(juce::Font(juce::FontOptions(12.5f).withStyle("Bold")));
        g.setColour(juce::Colour(0xffffffff));
        g.drawText(currentPresetName, topArea.toNearestInt(), juce::Justification::centred, false);

        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.setColour(juce::Colour(0xff757885));
        g.drawText(isFactoryPreset ? "Factory Preset" : "User Preset", textArea.toNearestInt(), juce::Justification::centred, false);

        // 3. Right: Producer Toy Headphone Mascot & Angled Grip Lines
        const float rightX = bounds.getRight() - 18.0f;

        // 3 Angled grip lines on far right
        g.setColour(juce::Colour(0x35ffffff));
        for (int i = 0; i < 3; ++i)
        {
            const float lineX = rightX - static_cast<float>(i) * 5.0f;
            g.drawLine(lineX - 8.0f, bounds.getY() + 38.0f, lineX, bounds.getY() + 24.0f, 1.6f);
        }

        // Mascot Bear Head with Headphones
        if (mascotImage.isValid())
        {
            const float mascotSize = 42.0f;
            const float mascotX = rightX - 32.0f - mascotSize;
            const float mascotY = bounds.getY() + (bounds.getHeight() - mascotSize) * 0.5f;

            g.drawImage(mascotImage, static_cast<int>(mascotX), static_cast<int>(mascotY),
                        static_cast<int>(mascotSize), static_cast<int>(mascotSize),
                        0, 0, mascotImage.getWidth(), mascotImage.getHeight());
        }
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        // Custom draw icons on Save, Delete, and Settings buttons
        drawIconButton(g, saveBtn.getBounds().toFloat(), 0);     // Save (Floppy Disk)
        drawIconButton(g, deleteBtn.getBounds().toFloat(), 1);   // Delete (Trash)
        drawIconButton(g, settingsBtn.getBounds().toFloat(), 2); // Settings (Gear)
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();

        // Center Preset Bar (Width ~210px, Height ~34px)
        const int barW = 210;
        const int barH = 34;
        const int barX = bounds.getCentreX() - (barW / 2) - 35;
        const int barY = bounds.getCentreY() - (barH / 2);
        presetBarBounds = juce::Rectangle<int>(barX, barY, barW, barH);

        prevBtn.setBounds(barX + 4, barY + 4, 26, barH - 8);
        nextBtn.setBounds(barX + barW - 30, barY + 4, 26, barH - 8);

        // Action icon buttons next to preset bar
        const int iconBtnSize = 28;
        const int iconBtnY = bounds.getCentreY() - (iconBtnSize / 2);
        int iconX = barX + barW + 10;

        saveBtn.setBounds(iconX, iconBtnY, iconBtnSize, iconBtnSize);
        iconX += iconBtnSize + 6;

        deleteBtn.setBounds(iconX, iconBtnY, iconBtnSize, iconBtnSize);
        iconX += iconBtnSize + 6;

        settingsBtn.setBounds(iconX, iconBtnY, iconBtnSize, iconBtnSize);
    }

private:
    PresetManager& presetManager;

    juce::Image logoImage;
    juce::Image wordmarkImage;
    juce::Image mascotImage;

    juce::Rectangle<int> presetBarBounds;
    juce::String currentPresetName { "Default" };
    bool isFactoryPreset { true };

    juce::TextButton prevBtn;
    juce::TextButton nextBtn;
    juce::TextButton saveBtn;
    juce::TextButton deleteBtn;
    juce::TextButton settingsBtn;

    void drawIconButton(juce::Graphics& g, const juce::Rectangle<float>& r, int iconType)
    {
        // Button background
        g.setColour(juce::Colour(0xff16171b));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(juce::Colour(0xff272932));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);

        const float cx = r.getCentreX();
        const float cy = r.getCentreY();
        g.setColour(juce::Colour(0xffc5c8d4));

        if (iconType == 0)
        {
            // Floppy disk save icon
            juce::Path p;
            p.addRoundedRectangle(cx - 6.0f, cy - 6.0f, 12.0f, 12.0f, 1.5f);
            g.strokePath(p, juce::PathStrokeType(1.4f));
            g.fillRect(cx - 3.5f, cy - 5.5f, 7.0f, 4.0f);
            g.fillRect(cx - 4.0f, cy + 1.0f, 8.0f, 4.0f);
        }
        else if (iconType == 1)
        {
            // Trash can icon
            g.drawRect(cx - 5.0f, cy - 3.0f, 10.0f, 9.0f, 1.4f);
            g.drawLine(cx - 7.0f, cy - 3.5f, cx + 7.0f, cy - 3.5f, 1.4f);
            g.drawLine(cx - 2.5f, cy - 5.0f, cx + 2.5f, cy - 5.0f, 1.4f);
            g.drawLine(cx - 2.0f, cy - 0.5f, cx - 2.0f, cy + 4.5f, 1.2f);
            g.drawLine(cx + 2.0f, cy - 0.5f, cx + 2.0f, cy + 4.5f, 1.2f);
        }
        else if (iconType == 2)
        {
            // Gear icon
            g.drawEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f, 1.5f);
            g.fillEllipse(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
            for (int i = 0; i < 6; ++i)
            {
                const float a = static_cast<float>(i) * (juce::MathConstants<float>::pi / 3.0f);
                g.drawLine(cx + std::cos(a) * 4.5f, cy + std::sin(a) * 4.5f,
                           cx + std::cos(a) * 6.5f, cy + std::sin(a) * 6.5f, 1.8f);
            }
        }
    }
};

} // namespace toytune
