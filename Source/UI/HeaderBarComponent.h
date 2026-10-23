#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BinaryData.h"
#include "../Presets/PresetManager.h"
#include "../Parameters/ToyTuneParameters.h"

namespace toytune
{

class HeaderBarComponent : public juce::Component
{
public:
    HeaderBarComponent(juce::AudioProcessorValueTreeState& apvts, PresetManager& pManager)
        : presetManager(pManager)
    {
        // Load embedded logos
        logoImage = juce::ImageCache::getFromMemory(BinaryData::producertoycharcoallogo_png,
                                                   BinaryData::producertoycharcoallogo_pngSize);
        wordmarkImage = juce::ImageCache::getFromMemory(BinaryData::toytunewordmark_png,
                                                       BinaryData::toytunewordmark_pngSize);

        // Preset Selector
        presetBox.addItemList(presetManager.getPresetNames(), 1);
        presetBox.setSelectedId(presetManager.getCurrentPresetIndex() + 1, juce::dontSendNotification);
        presetBox.onChange = [this]()
        {
            const int idx = presetBox.getSelectedId() - 1;
            if (idx >= 0)
                presetManager.applyPreset(idx);
        };
        addAndMakeVisible(presetBox);

        // Save Preset Button
        saveBtn.setButtonText("Save");
        saveBtn.onClick = [this]()
        {
            // Prompt user for preset name
            auto* parent = getTopLevelComponent();
            juce::AlertWindow::showOkCancelBox(
                juce::AlertWindow::QuestionIcon,
                "Save Preset",
                "Enter preset name:",
                "Save", "Cancel",
                parent,
                juce::ModalCallbackFunction::create([this](int result)
                {
                    if (result != 0)
                    {
                        presetManager.saveUserPreset("MyVocalTune");
                    }
                }));
        };
        addAndMakeVisible(saveBtn);

        // Bypass Toggle Button
        bypassBtn.setButtonText("Bypass");
        bypassBtn.setClickingTogglesState(true);
        bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParameterIDs::bypass, bypassBtn);
        addAndMakeVisible(bypassBtn);
    }

    void updatePresetSelection(int index)
    {
        presetBox.setSelectedId(index + 1, juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Header Background
        g.setColour(juce::Colour(0xff111114));
        g.fillRect(bounds);
        g.setColour(juce::Colour(0xff222228));
        g.drawHorizontalLine(static_cast<int>(bounds.getBottom() - 1.0f), 0.0f, bounds.getWidth());

        // Left Branding: Producer Toy Logo + ToyTune Wordmark
        float currentX = 14.0f;
        if (logoImage.isValid())
        {
            const float logoW = 34.0f;
            const float logoH = 34.0f;
            g.drawImage(logoImage, static_cast<int>(currentX), static_cast<int>((bounds.getHeight() - logoH) * 0.5f),
                        static_cast<int>(logoW), static_cast<int>(logoH),
                        0, 0, logoImage.getWidth(), logoImage.getHeight());
            currentX += logoW + 10.0f;
        }

        if (wordmarkImage.isValid())
        {
            const float wmH = 26.0f;
            const float wmW = wmH * (static_cast<float>(wordmarkImage.getWidth()) / static_cast<float>(wordmarkImage.getHeight()));
            g.drawImage(wordmarkImage, static_cast<int>(currentX), static_cast<int>((bounds.getHeight() - wmH) * 0.5f),
                        static_cast<int>(wmW), static_cast<int>(wmH),
                        0, 0, wordmarkImage.getWidth(), wordmarkImage.getHeight());
            currentX += wmW + 12.0f;
        }
        else
        {
            // Text Fallback
            g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
            g.setColour(juce::Colour(0xffffffff));
            g.drawText("ToyTune", static_cast<int>(currentX), 0, 100, static_cast<int>(bounds.getHeight()), juce::Justification::centredLeft);
            currentX += 105.0f;
        }

        g.setFont(juce::Font(juce::FontOptions(10.5f)));
        g.setColour(juce::Colour(0xff6e6e78));
        g.drawText("by Producer Toy", static_cast<int>(currentX), 0, 110, static_cast<int>(bounds.getHeight()), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 8);

        bypassBtn.setBounds(area.removeFromRight(64));
        area.removeFromRight(12);

        saveBtn.setBounds(area.removeFromRight(52));
        area.removeFromRight(8);

        presetBox.setBounds(area.removeFromRight(150));
    }

private:
    juce::Image logoImage;
    juce::Image wordmarkImage;

    juce::ComboBox presetBox;
    juce::TextButton saveBtn;
    juce::ToggleButton bypassBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    PresetManager& presetManager;
};

} // namespace toytune
