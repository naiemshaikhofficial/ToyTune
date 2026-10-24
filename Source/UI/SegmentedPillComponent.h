#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace toytune
{

/**
 * Custom Segmented Pill Selector matching Producer Toy ToyTune reference image.
 * Active segment renders as a bright white pill with black text.
 * Inactive segments render as dark charcoal with silver text.
 */
class SegmentedPillComponent : public juce::Component
{
public:
    SegmentedPillComponent(const juce::StringArray& segmentLabels,
                           juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& paramId,
                           const std::vector<int>& paramValues)
        : labels(segmentLabels),
          state(apvts),
          parameterId(paramId),
          values(paramValues)
    {
        for (int i = 0; i < labels.size(); ++i)
        {
            auto btn = std::make_unique<juce::TextButton>(labels[i]);
            btn->setClickingTogglesState(false);
            btn->setConnectedEdges(0);
            btn->onClick = [this, i]()
            {
                selectSegment(i, true);
            };
            addAndMakeVisible(*btn);
            buttons.push_back(std::move(btn));
        }

        // Initialize from APVTS parameter value
        if (auto* param = state.getRawParameterValue(parameterId))
        {
            const int currentVal = static_cast<int>(std::round(param->load()));
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (values[i] == currentVal)
                {
                    selectedSegment = static_cast<int>(i);
                    break;
                }
            }
        }
    }

    std::function<void(int)> onSelectionChanged;

    void selectSegment(int index, bool notify)
    {
        if (index >= 0 && index < labels.size())
        {
            selectedSegment = index;
            if (notify && index < static_cast<int>(values.size()))
            {
                if (auto* p = state.getParameter(parameterId))
                {
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(values[static_cast<size_t>(index)])));
                }
            }
            if (onSelectionChanged != nullptr)
                onSelectionChanged(selectedSegment);
            repaint();
        }
    }

    void updateFromParameter()
    {
        if (auto* param = state.getRawParameterValue(parameterId))
        {
            const int currentVal = static_cast<int>(std::round(param->load()));
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (values[i] == currentVal)
                {
                    if (selectedSegment != static_cast<int>(i))
                    {
                        selectedSegment = static_cast<int>(i);
                        if (onSelectionChanged != nullptr)
                            onSelectionChanged(selectedSegment);
                        repaint();
                    }
                    break;
                }
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Dark pill background
        g.setColour(juce::Colour(0xff121316));
        g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);
        g.setColour(juce::Colour(0xff23252c));
        g.drawRoundedRectangle(bounds, bounds.getHeight() * 0.5f, 1.2f);
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        const int count = static_cast<int>(buttons.size());
        if (count == 0) return;

        for (int i = 0; i < count; ++i)
        {
            const auto btnBounds = buttons[static_cast<size_t>(i)]->getBounds().toFloat().reduced(2.0f);
            const bool isActive = (i == selectedSegment);

            if (isActive)
            {
                // Active segment: Bright white pill with dark text
                g.setColour(juce::Colour(0xffffffff));
                g.fillRoundedRectangle(btnBounds, btnBounds.getHeight() * 0.5f);

                g.setColour(juce::Colour(0xff0d0e12));
                g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
                g.drawText(labels[i], btnBounds, juce::Justification::centred, false);
            }
            else
            {
                // Inactive segment: Silver text
                g.setColour(juce::Colour(0xff9da1ad));
                g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Regular")));
                g.drawText(labels[i], btnBounds, juce::Justification::centred, false);
            }
        }
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        const int count = static_cast<int>(buttons.size());
        if (count == 0) return;

        const int segW = bounds.getWidth() / count;
        for (int i = 0; i < count; ++i)
        {
            buttons[static_cast<size_t>(i)]->setBounds(i * segW, 0, segW, bounds.getHeight());
            buttons[static_cast<size_t>(i)]->setAlpha(0.0f); // Button is clickable, painted in paintOverChildren
        }
    }

private:
    juce::StringArray labels;
    juce::AudioProcessorValueTreeState& state;
    juce::String parameterId;
    std::vector<int> values;
    int selectedSegment { 0 };

    std::vector<std::unique_ptr<juce::TextButton>> buttons;
};

} // namespace toytune
