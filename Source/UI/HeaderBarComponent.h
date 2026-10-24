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

        juce::Graphics::ScopedSaveState state(g);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

        // 1. Left Branding: PRODUCER TOY Logo + Divider + ToyTune 3D Logo + Subtitle
        float curX = bounds.getX() + 24.0f;
        const float logoMaxH = juce::jlimit(34.0f, 48.0f, bounds.getHeight() * 0.68f);

        if (logoImage.isValid())
        {
            const float ptAspect = static_cast<float>(logoImage.getWidth()) / static_cast<float>(std::max(1, logoImage.getHeight()));
            const float ptW = logoMaxH * ptAspect;
            const float ptY = bounds.getY() + (bounds.getHeight() - logoMaxH) * 0.5f;
            g.drawImageWithin(logoImage, static_cast<int>(curX), static_cast<int>(ptY),
                              static_cast<int>(ptW), static_cast<int>(logoMaxH),
                              juce::RectanglePlacement(juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid | juce::RectanglePlacement::onlyReduceInSize));
            curX += ptW + 16.0f;
        }

        // Vertical metallic hairline divider
        g.setColour(juce::Colour(0x35ffffff));
        g.drawVerticalLine(static_cast<int>(curX), bounds.getY() + 14.0f, bounds.getBottom() - 14.0f);
        curX += 16.0f;

        // ToyTune 3D Wordmark Logo
        if (wordmarkImage.isValid())
        {
            const float ttAspect = static_cast<float>(wordmarkImage.getWidth()) / static_cast<float>(std::max(1, wordmarkImage.getHeight()));
            const float ttH = logoMaxH * 0.78f;
            const float ttW = ttH * ttAspect;
            const float ttY = bounds.getY() + (bounds.getHeight() - (ttH + 13.0f)) * 0.5f;

            g.drawImageWithin(wordmarkImage, static_cast<int>(curX), static_cast<int>(ttY),
                              static_cast<int>(ttW), static_cast<int>(ttH),
                              juce::RectanglePlacement(juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid | juce::RectanglePlacement::onlyReduceInSize));

            // Subtitle: REAL-TIME VOCAL TUNE
            g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff858895));
            g.drawText("REAL-TIME  VOCAL  TUNE",
                       static_cast<int>(curX + 2.0f),
                       static_cast<int>(ttY + ttH + 1.0f),
                       static_cast<int>(ttW + 30.0f),
                       12,
                       juce::Justification::centredLeft, false);
        }
        else
        {
            // Typographic fallback if image missing
            const float textY = bounds.getY() + (bounds.getHeight() - 36.0f) * 0.5f;
            g.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
            g.setColour(juce::Colour(0xffffffff));
            g.drawText("Toy", static_cast<int>(curX), static_cast<int>(textY), 40, 24, juce::Justification::centredLeft, false);

            juce::ColourGradient tuneGrad(juce::Colour(0xffffffff), curX + 38.0f, textY,
                                          juce::Colour(0xffb5b8c5), curX + 38.0f, textY + 24.0f, false);
            g.setGradientFill(tuneGrad);
            g.drawText("Tune", static_cast<int>(curX + 38.0f), static_cast<int>(textY), 65, 24, juce::Justification::centredLeft, false);

            g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff828590));
            g.drawText("REAL-TIME  VOCAL  TUNE", static_cast<int>(curX + 1.0f), static_cast<int>(textY + 24.0f), 150, 12, juce::Justification::centredLeft, false);
        }

        // 2. Center Preset Bar Container
        const auto presetBarRect = presetBarBounds.toFloat();
        g.setColour(juce::Colour(0xff121316));
        g.fillRoundedRectangle(presetBarRect, 8.0f);
        g.setColour(juce::Colour(0xff23252c));
        g.drawRoundedRectangle(presetBarRect, 8.0f, 1.2f);

        // Preset Name & Subtitle in Center
        auto textArea = presetBarRect.reduced(38.0f, 3.0f);
        const auto topArea = textArea.removeFromTop(18.0f);
        g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
        g.setColour(juce::Colour(0xffffffff));
        g.drawText(currentPresetName, topArea.toNearestInt(), juce::Justification::centred, false);

        g.setFont(juce::Font(juce::FontOptions(9.5f)));
        g.setColour(juce::Colour(0xff757885));
        g.drawText(isFactoryPreset ? "Factory Preset" : "User Preset", textArea.toNearestInt(), juce::Justification::centred, false);

        // 3. Right: Borderless Producer Toy Headphone Mascot & Angled Grip Lines
        const float rightMargin = 22.0f;
        const float rightX = bounds.getRight() - rightMargin;

        // 3 Angled grip lines on far right
        g.setColour(juce::Colour(0x35ffffff));
        for (int i = 0; i < 3; ++i)
        {
            const float lineX = rightX - static_cast<float>(i) * 6.0f;
            g.drawLine(lineX - 9.0f, bounds.getY() + bounds.getHeight() * 0.62f,
                       lineX, bounds.getY() + bounds.getHeight() * 0.38f, 1.8f);
        }

        // Borderless Mascot Bear Head with Headphones
        if (mascotImage.isValid())
        {
            const float mascotSize = juce::jlimit(42.0f, 56.0f, bounds.getHeight() * 0.78f);
            const float mascotX = rightX - 36.0f - mascotSize;
            const float mascotY = bounds.getY() + (bounds.getHeight() - mascotSize) * 0.5f;

            g.drawImageWithin(mascotImage, static_cast<int>(mascotX), static_cast<int>(mascotY),
                              static_cast<int>(mascotSize), static_cast<int>(mascotSize),
                              juce::RectanglePlacement(juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize));
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

        // Preset bar: width 230-310px, height 34-40px, centered
        const int barW = juce::jlimit(230, 310, static_cast<int>(bounds.getWidth() * 0.22f));
        const int barH = juce::jlimit(34, 40, static_cast<int>(bounds.getHeight() * 0.58f));
        const int barX = bounds.getCentreX() - (barW / 2) - 45;
        const int barY = bounds.getCentreY() - (barH / 2);
        presetBarBounds = juce::Rectangle<int>(barX, barY, barW, barH);

        prevBtn.setBounds(barX + 4, barY + 4, 28, barH - 8);
        nextBtn.setBounds(barX + barW - 32, barY + 4, 28, barH - 8);

        // Action icon buttons next to preset bar
        const int iconBtnSize = barH - 6;
        const int iconBtnY = bounds.getCentreY() - (iconBtnSize / 2);
        int iconX = barX + barW + 12;

        saveBtn.setBounds(iconX, iconBtnY, iconBtnSize, iconBtnSize);
        iconX += iconBtnSize + 8;

        deleteBtn.setBounds(iconX, iconBtnY, iconBtnSize, iconBtnSize);
        iconX += iconBtnSize + 8;

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
