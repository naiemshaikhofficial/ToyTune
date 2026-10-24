#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ToyTuneParameters.h"
#include "../PitchCorrection/ScaleEngine.h"
#include "RotarySliderWithLabel.h"
#include <array>
#include <functional>

namespace toytune
{

/**
 * Interactive Cents Detune Cell matching Auto-Tune Pro 11 Scale View.
 * Displays numerical cents detune (-50 to +50 cents).
 * Click & drag up/down to adjust, double-click to reset to 0.
 * Styled in dark luxury monochrome aesthetic with bright white illumination when detuned.
 */
class NoteCentsCell : public juce::Component
{
public:
    NoteCentsCell()
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }

    void setValue(float val)
    {
        val = juce::jlimit(-50.0f, 50.0f, val);
        if (std::abs(centsValue - val) > 0.01f)
        {
            centsValue = val;
            repaint();
        }
    }

    float getValue() const noexcept { return centsValue; }

    std::function<void(float)> onValueChanged;

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const bool isNonZero = std::abs(centsValue) > 0.5f;

        // Cell background
        g.setColour(isMouseOver ? juce::Colour(0xff1e2027) : juce::Colour(0xff121316));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Cell border
        g.setColour(isNonZero ? juce::Colour(0xffffffff) : (isMouseOver ? juce::Colour(0xff3a3d4a) : juce::Colour(0xff22242a)));
        g.drawRoundedRectangle(bounds, 4.0f, isNonZero ? 1.2f : 1.0f);

        // Detune numeric readout
        juce::String str;
        if (centsValue > 0.5f)
            str = "+" + juce::String(static_cast<int>(std::round(centsValue)));
        else
            str = juce::String(static_cast<int>(std::round(centsValue)));

        g.setFont(juce::Font(juce::FontOptions(10.5f).withStyle(isNonZero ? "Bold" : "Regular")));
        g.setColour(isNonZero ? juce::Colour(0xffffffff) : juce::Colour(0xff8a8d9a));
        g.drawText(str, bounds, juce::Justification::centred, false);
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        isMouseOver = true;
        repaint();
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        isMouseOver = false;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragStartY = e.position.y;
        centsAtDragStart = centsValue;
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        const float deltaY = dragStartY - e.position.y;
        const float sensitivity = e.mods.isShiftDown() ? 0.2f : 0.8f;
        const float newVal = juce::jlimit(-50.0f, 50.0f, std::round(centsAtDragStart + deltaY * sensitivity));
        if (newVal != centsValue)
        {
            setValue(newVal);
            if (onValueChanged)
                onValueChanged(centsValue);
        }
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        setValue(0.0f);
        if (onValueChanged)
            onValueChanged(0.0f);
    }

private:
    float centsValue { 0.0f };
    float dragStartY { 0.0f };
    float centsAtDragStart { 0.0f };
    bool isMouseOver { false };
};

/**
 * Custom Note Pill Button for Bypass and Remove rows matching Auto-Tune Pro 11.
 * Features crisp dark luxury monochrome styling with high-contrast active states.
 */
class ScaleNoteButton : public juce::Button
{
public:
    enum class ButtonRole { Bypass, Remove };

    ScaleNoteButton(const juce::String& noteName, ButtonRole role)
        : juce::Button(noteName), text(noteName), role(role)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool /*shouldDrawButtonAsDown*/) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const bool toggled = getToggleState();

        if (role == ButtonRole::Bypass)
        {
            // Bypass Button: toggled = BYPASSED (Active Bypass)
            if (toggled)
            {
                // Active Bypass: Illuminated border & bright text
                g.setColour(juce::Colour(0xff22242c));
                g.fillRoundedRectangle(bounds, 4.0f);
                g.setColour(juce::Colour(0xffffffff));
                g.drawRoundedRectangle(bounds, 4.0f, 1.2f);
                g.setColour(juce::Colour(0xffffffff));
            }
            else
            {
                // Untuned / Normal: Dark subtle background
                g.setColour(shouldDrawButtonAsHighlighted ? juce::Colour(0xff181a20) : juce::Colour(0xff121316));
                g.fillRoundedRectangle(bounds, 4.0f);
                g.setColour(juce::Colour(0xff22242a));
                g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
                g.setColour(juce::Colour(0xff7a7d8c));
            }
        }
        else // Remove Button: toggled = IN SCALE (Active note in quantization mask)
        {
            if (toggled)
            {
                // Note is ACTIVE in scale: Solid illuminated pill with bright white bold text
                g.setColour(shouldDrawButtonAsHighlighted ? juce::Colour(0xff252732) : juce::Colour(0xff1c1e25));
                g.fillRoundedRectangle(bounds, 4.0f);
                g.setColour(juce::Colour(0xff3a3d4a));
                g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
                g.setColour(juce::Colour(0xffffffff));
            }
            else
            {
                // Note is REMOVED from scale: Deep dark background with dimmed grey text
                g.setColour(juce::Colour(0xff0d0e11));
                g.fillRoundedRectangle(bounds, 4.0f);
                g.setColour(juce::Colour(0xff16171b));
                g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
                g.setColour(juce::Colour(0xff454854));
            }
        }

        g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        g.drawText(text, bounds, juce::Justification::centred, false);
    }

private:
    juce::String text;
    ButtonRole role;
};

/**
 * Auto-Tune Pro 11-Grade Advanced Drawer Component for ToyTune.
 * Features:
 * 1. Collapsed State: Single centered pill tab `[ Advanced ︽ ]` resting above chromatic keyboard.
 * 2. Expanded State (192px): Defaults to `Scale` view. Header expands to `[ Vibrato ] | [ Advanced ︾ ] | [ Scale ]`.
 * 3. Scale View: Set Column (Major, Minor, All) + 12 Note Matrix (Cents, Bypass, Remove) + Ignore Vibrato + MIDI Controls.
 * 4. Vibrato View: LFO Generator (2x2 Shape Grid + Rate, Delay, Onset, Variation) + Modulate Targets (Pitch, Amp, Formant).
 * Strict adherence to Producer Toy dark luxury monochrome design system.
 */
class AdvancedDrawerComponent : public juce::Component
{
public:
    AdvancedDrawerComponent(juce::AudioProcessorValueTreeState& state, ScaleEngine& scaleEng)
        : apvts(state),
          scaleEngine(scaleEng),
          rateKnob("Rate", "Hz", true),
          delayKnob("Delay", "ms", false),
          onsetKnob("Onset Rate", "ms", false),
          varKnob("Variation", "%", false),
          pitchModKnob("Pitch", "ct", false),
          ampModKnob("Amplitude", "%", false),
          formantModKnob("Formant", "%", false)
    {
        // 1. Header Bar Controls
        vibratoTabBtn.setButtonText("Vibrato");
        vibratoTabBtn.setClickingTogglesState(false);
        vibratoTabBtn.onClick = [this]()
        {
            if (!isOpen)
                setDrawerOpen(true);
            setActiveTab(0);
        };
        addAndMakeVisible(vibratoTabBtn);

        scaleTabBtn.setButtonText("Scale");
        scaleTabBtn.setClickingTogglesState(false);
        scaleTabBtn.onClick = [this]()
        {
            if (!isOpen)
                setDrawerOpen(true);
            setActiveTab(1);
        };
        addAndMakeVisible(scaleTabBtn);

        advancedToggleBtn.onClick = [this]()
        {
            if (!isOpen)
            {
                // By default when opening, open directly to Scale tab as specified!
                activeTab = 1;
                if (auto* param = apvts.getParameter(ParameterIDs::advActiveTab))
                    param->setValueNotifyingHost(param->convertTo0to1(1.0f));
            }
            setDrawerOpen(!isOpen);
        };
        addAndMakeVisible(advancedToggleBtn);

        // 2. Scale Tab Components (Image 3)
        majorSetBtn.setButtonText("Major");
        majorSetBtn.onClick = [this]() { applyQuickSet(0); };
        addAndMakeVisible(majorSetBtn);

        minorSetBtn.setButtonText("Minor");
        minorSetBtn.onClick = [this]() { applyQuickSet(1); };
        addAndMakeVisible(minorSetBtn);

        allSetBtn.setButtonText("All");
        allSetBtn.onClick = [this]() { applyQuickSet(2); };
        addAndMakeVisible(allSetBtn);

        // 12 Note Columns
        const auto noteNames = ToyTuneParameters::getRootNoteNames();
        for (int i = 0; i < 12; ++i)
        {
            // Row 1: Interactive Cents Cell
            centsCells[i] = std::make_unique<NoteCentsCell>();
            if (auto* param = apvts.getRawParameterValue(ParameterIDs::noteCents(i)))
                centsCells[i]->setValue(param->load());

            const int noteIdx = i;
            centsCells[i]->onValueChanged = [this, noteIdx](float val)
            {
                if (auto* param = apvts.getParameter(ParameterIDs::noteCents(noteIdx)))
                    param->setValueNotifyingHost(param->convertTo0to1(val));
                scaleEngine.setCentsDetune(noteIdx, val);
            };
            addAndMakeVisible(*centsCells[i]);

            // Row 2: Bypass Button
            bypButtons[i] = std::make_unique<ScaleNoteButton>(noteNames[i], ScaleNoteButton::ButtonRole::Bypass);
            bypAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, ParameterIDs::noteBypass(i), *bypButtons[i]);
            addAndMakeVisible(*bypButtons[i]);

            // Row 3: Remove Button
            remButtons[i] = std::make_unique<ScaleNoteButton>(noteNames[i], ScaleNoteButton::ButtonRole::Remove);
            remButtons[i]->onClick = [this, noteIdx]()
            {
                if (auto* param = apvts.getParameter(ParameterIDs::noteMask(noteIdx)))
                {
                    const bool currentlyActive = param->getValue() > 0.5f;
                    param->setValueNotifyingHost(currentlyActive ? 0.0f : 1.0f);
                }
                updateRemButtonsUI();
            };
            addAndMakeVisible(*remButtons[i]);
        }

        // Action Buttons on right of note rows
        zeroCentsBtn.setButtonText("Reset");
        zeroCentsBtn.onClick = [this]()
        {
            for (int i = 0; i < 12; ++i)
            {
                if (auto* param = apvts.getParameter(ParameterIDs::noteCents(i)))
                    param->setValueNotifyingHost(param->convertTo0to1(0.0f));
                if (centsCells[i])
                    centsCells[i]->setValue(0.0f);
                scaleEngine.setCentsDetune(i, 0.0f);
            }
        };
        addAndMakeVisible(zeroCentsBtn);

        bypAllBtn.setButtonText("All");
        bypAllBtn.onClick = [this]() { toggleAllBypass(); };
        addAndMakeVisible(bypAllBtn);

        remAllBtn.setButtonText("All");
        remAllBtn.onClick = [this]() { toggleAllRemove(); };
        addAndMakeVisible(remAllBtn);

        // Ignore Vibrato Button
        ignoreVibBtn.setButtonText(juce::String::charToString(0x223F) + " Ignore Vibrato");
        ignoreVibBtn.setClickingTogglesState(true);
        ignoreVibBtn.onClick = [this]()
        {
            ignoreVibratoActive = !ignoreVibratoActive;
            ignoreVibBtn.setToggleState(ignoreVibratoActive, juce::dontSendNotification);
            updateIgnoreVibUI();
        };
        addAndMakeVisible(ignoreVibBtn);

        // MIDI Rack Buttons
        midiToNotesBtn.setButtonText("To Notes");
        midiToNotesBtn.setClickingTogglesState(true);
        midiToNotesBtn.onClick = [this]() { updateMidiButtonsUI(); };
        addAndMakeVisible(midiToNotesBtn);

        midiLearnScaleBtn.setButtonText("Learn Scale");
        midiLearnScaleBtn.setClickingTogglesState(true);
        midiLearnScaleBtn.onClick = [this]() { updateMidiButtonsUI(); };
        addAndMakeVisible(midiLearnScaleBtn);

        midiAllOctavesBtn.setButtonText("All Octaves");
        midiAllOctavesBtn.setClickingTogglesState(true);
        midiAllOctavesBtn.onClick = [this]() { updateMidiButtonsUI(); };
        addAndMakeVisible(midiAllOctavesBtn);

        // 3. Vibrato Tab Components (Image 4)
        // 2x2 Shape Selector: Off, Sine ~, Saw ⩘, Square ⎍
        static const juce::String shapeLabels[] = {
            "Off",
            "Sine " + juce::String::charToString(0x223F),
            "Saw " + juce::String::charToString(0x22BF),
            "Square " + juce::String::charToString(0x2317)
        };
        for (int i = 0; i < 4; ++i)
        {
            shapeButtons[i] = std::make_unique<juce::TextButton>(shapeLabels[i]);
            shapeButtons[i]->setClickingTogglesState(false);
            const int shapeIdx = i;
            shapeButtons[i]->onClick = [this, shapeIdx]()
            {
                if (auto* param = apvts.getParameter(ParameterIDs::advVibShape))
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(shapeIdx)));
                updateShapeButtonsUI();
            };
            addAndMakeVisible(*shapeButtons[i]);
        }

        // LFO Knobs
        rateKnob.attachToParameter(apvts, ParameterIDs::advVibRate);
        addAndMakeVisible(rateKnob);

        delayKnob.attachToParameter(apvts, ParameterIDs::advVibDelay);
        addAndMakeVisible(delayKnob);

        onsetKnob.attachToParameter(apvts, ParameterIDs::advVibOnset);
        addAndMakeVisible(onsetKnob);

        varKnob.attachToParameter(apvts, ParameterIDs::advVibVariation);
        addAndMakeVisible(varKnob);

        pitchModKnob.attachToParameter(apvts, ParameterIDs::advVibPitch);
        addAndMakeVisible(pitchModKnob);

        ampModKnob.attachToParameter(apvts, ParameterIDs::advVibAmp);
        addAndMakeVisible(ampModKnob);

        formantModKnob.attachToParameter(apvts, ParameterIDs::advVibFormant);
        addAndMakeVisible(formantModKnob);

        // Read initial state from APVTS
        if (auto* openVal = apvts.getRawParameterValue(ParameterIDs::advDrawerOpen))
            isOpen = openVal->load() > 0.5f;

        if (auto* tabVal = apvts.getRawParameterValue(ParameterIDs::advActiveTab))
            activeTab = static_cast<int>(std::round(tabVal->load()));
        else
            activeTab = 1; // Default to Scale

        updateHeaderUI();
        updateVisibility();
        updateShapeButtonsUI();
        updateRemButtonsUI();
        updateCentsUI();
        updateMidiButtonsUI();
        updateIgnoreVibUI();
    }

    void setDrawerOpen(bool open)
    {
        if (isOpen != open)
        {
            isOpen = open;
            if (isOpen)
            {
                // When expanding, default to Scale tab (Index 1) as requested by user
                activeTab = 1;
                if (auto* param = apvts.getParameter(ParameterIDs::advActiveTab))
                    param->setValueNotifyingHost(param->convertTo0to1(1.0f));
            }

            if (auto* param = apvts.getParameter(ParameterIDs::advDrawerOpen))
                param->setValueNotifyingHost(isOpen ? 1.0f : 0.0f);

            updateHeaderUI();
            updateVisibility();
            if (onDrawerToggled)
                onDrawerToggled(isOpen);

            resized();
            repaint();
        }
    }

    bool isDrawerOpen() const noexcept { return isOpen; }

    void setActiveTab(int tabIndex)
    {
        activeTab = juce::jlimit(0, 1, tabIndex);
        if (auto* param = apvts.getParameter(ParameterIDs::advActiveTab))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(activeTab)));

        updateHeaderUI();
        updateVisibility();
        resized();
        repaint();
    }

    int getActiveTab() const noexcept { return activeTab; }

    int getDesiredHeight() const noexcept
    {
        return isOpen ? 192 : 32;
    }

    std::function<void(bool)> onDrawerToggled;

    void updateRemButtonsUI()
    {
        for (int i = 0; i < 12; ++i)
        {
            if (auto* param = apvts.getRawParameterValue(ParameterIDs::noteMask(i)))
            {
                const bool isActiveInScale = param->load() > 0.5f;
                // Button toggle state is true when note is IN SCALE (illuminated)
                remButtons[i]->setToggleState(isActiveInScale, juce::dontSendNotification);
            }
        }
    }

    void updateCentsUI()
    {
        for (int i = 0; i < 12; ++i)
        {
            if (auto* param = apvts.getRawParameterValue(ParameterIDs::noteCents(i)))
            {
                if (centsCells[i])
                    centsCells[i]->setValue(param->load());
            }
        }
    }

    void updateShapeButtonsUI()
    {
        int currentShape = 0;
        if (auto* param = apvts.getRawParameterValue(ParameterIDs::advVibShape))
            currentShape = static_cast<int>(std::round(param->load()));

        const juce::Colour activeBg(0xffffffff);
        const juce::Colour inactiveBg(0xff141518);
        const juce::Colour activeText(0xff0a0a0d);
        const juce::Colour inactiveText(0xff8a8d9a);

        for (int i = 0; i < 4; ++i)
        {
            const bool isSelected = (i == currentShape);
            shapeButtons[i]->setToggleState(isSelected, juce::dontSendNotification);
            shapeButtons[i]->setColour(juce::TextButton::buttonColourId, isSelected ? activeBg : inactiveBg);
            shapeButtons[i]->setColour(juce::TextButton::textColourOnId, isSelected ? activeText : inactiveText);
            shapeButtons[i]->setColour(juce::TextButton::textColourOffId, isSelected ? activeText : inactiveText);
        }
    }

    void updateMidiButtonsUI()
    {
        auto styleBtn = [](juce::TextButton& btn)
        {
            const bool on = btn.getToggleState();
            btn.setColour(juce::TextButton::buttonColourId, on ? juce::Colour(0xff22242c) : juce::Colour(0xff141518));
            btn.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
            btn.setColour(juce::TextButton::textColourOffId, on ? juce::Colour(0xffffffff) : juce::Colour(0xff8a8d9a));
        };
        styleBtn(midiToNotesBtn);
        styleBtn(midiLearnScaleBtn);
        styleBtn(midiAllOctavesBtn);
    }

    void updateIgnoreVibUI()
    {
        ignoreVibBtn.setColour(juce::TextButton::buttonColourId, ignoreVibratoActive ? juce::Colour(0xff22242c) : juce::Colour(0xff141518));
        ignoreVibBtn.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
        ignoreVibBtn.setColour(juce::TextButton::textColourOffId, ignoreVibratoActive ? juce::Colour(0xffffffff) : juce::Colour(0xff8a8d9a));
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // 1. Overall Drawer Container Background & Border
        g.setColour(juce::Colour(0xff0c0d10));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(juce::Colour(0xff1a1b22));
        g.drawRoundedRectangle(bounds, 8.0f, 1.2f);

        // 2. Header Divider Line
        const float headerH = 32.0f;
        g.setColour(juce::Colour(0xff16171e));
        g.drawLine(bounds.getX(), bounds.getY() + headerH, bounds.getRight(), bounds.getY() + headerH, 1.0f);

        if (!isOpen)
            return;

        // 3. Tab Specific Renderings
        const auto contentArea = bounds.withTrimmedTop(headerH + 4.0f).reduced(12.0f, 4.0f);

        if (activeTab == 0)
        {
            // Vibrato View Layout (Image 4)
            const float arrowW = 32.0f;
            const float availableW = contentArea.getWidth() - arrowW - 16.0f;
            const float createW = availableW * 0.58f;
            const float modW = availableW - createW;

            const auto createRect = juce::Rectangle<float>(contentArea.getX(), contentArea.getY(), createW, contentArea.getHeight());
            const auto arrowRect = juce::Rectangle<float>(createRect.getRight() + 8.0f, contentArea.getY(), arrowW, contentArea.getHeight());
            const auto modRect = juce::Rectangle<float>(arrowRect.getRight() + 8.0f, contentArea.getY(), modW, contentArea.getHeight());

            // Panel Backgrounds
            g.setColour(juce::Colour(0xff121317));
            g.fillRoundedRectangle(createRect, 6.0f);
            g.fillRoundedRectangle(modRect, 6.0f);

            g.setColour(juce::Colour(0xff1c1e25));
            g.drawRoundedRectangle(createRect, 6.0f, 1.0f);
            g.drawRoundedRectangle(modRect, 6.0f, 1.0f);

            // Group Titles (matching Auto-Tune Pro 11 centered lines)
            g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff8a8d9a));

            // Create Vibrato Header Line
            const float cvTextW = 110.0f;
            const float cvCenterX = createRect.getCentreX();
            const float cvTopY = createRect.getY() + 14.0f;
            g.drawLine(createRect.getX() + 16.0f, cvTopY, cvCenterX - (cvTextW / 2) - 6.0f, cvTopY, 1.0f);
            g.drawText("Create Vibrato", juce::Rectangle<float>(cvCenterX - cvTextW / 2, cvTopY - 8.0f, cvTextW, 16.0f), juce::Justification::centred, false);
            g.drawLine(cvCenterX + (cvTextW / 2) + 6.0f, cvTopY, createRect.getRight() - 16.0f, cvTopY, 1.0f);

            // Shape label above 2x2 grid
            g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff717482));
            g.drawText("Shape", juce::Rectangle<float>(createRect.getX() + 16.0f, createRect.getY() + 26.0f, 80.0f, 14.0f), juce::Justification::centredLeft, false);

            // Modulate Header Line
            const float modTextW = 75.0f;
            const float modCenterX = modRect.getCentreX();
            const float modTopY = modRect.getY() + 14.0f;
            g.drawLine(modRect.getX() + 16.0f, modTopY, modCenterX - (modTextW / 2) - 6.0f, modTopY, 1.0f);
            g.drawText("Modulate", juce::Rectangle<float>(modCenterX - modTextW / 2, modTopY - 8.0f, modTextW, 16.0f), juce::Justification::centred, false);
            g.drawLine(modCenterX + (modTextW / 2) + 6.0f, modTopY, modRect.getRight() - 16.0f, modTopY, 1.0f);

            // Center Signal Flow Routing Arrow
            juce::Path p;
            const float cy = arrowRect.getCentreY();
            const float cx = arrowRect.getCentreX();
            p.startNewSubPath(cx - 10.0f, cy);
            p.lineTo(cx + 8.0f, cy);
            p.startNewSubPath(cx + 2.0f, cy - 5.0f);
            p.lineTo(cx + 8.0f, cy);
            p.lineTo(cx + 2.0f, cy + 5.0f);
            g.setColour(juce::Colour(0xff454854));
            g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            // Scale View Layout (Image 3)
            const float setW = 88.0f;
            const float midiW = 94.0f;
            const float tableW = contentArea.getWidth() - setW - midiW - 16.0f;

            const auto setRect = juce::Rectangle<float>(contentArea.getX(), contentArea.getY(), setW, contentArea.getHeight());
            const auto tableRect = juce::Rectangle<float>(setRect.getRight() + 8.0f, contentArea.getY(), tableW, contentArea.getHeight());
            const auto midiRect = juce::Rectangle<float>(tableRect.getRight() + 8.0f, contentArea.getY(), midiW, contentArea.getHeight());

            // Card backgrounds
            g.setColour(juce::Colour(0xff121317));
            g.fillRoundedRectangle(setRect, 6.0f);
            g.fillRoundedRectangle(tableRect, 6.0f);
            g.fillRoundedRectangle(midiRect, 6.0f);

            g.setColour(juce::Colour(0xff1c1e25));
            g.drawRoundedRectangle(setRect, 6.0f, 1.0f);
            g.drawRoundedRectangle(tableRect, 6.0f, 1.0f);
            g.drawRoundedRectangle(midiRect, 6.0f, 1.0f);

            // Group Titles with bracket {
            g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff8a8d9a));
            g.drawText("Set  {", setRect.withTrimmedTop(6.0f).withHeight(18.0f).withTrimmedLeft(8.0f), juce::Justification::centredLeft, false);
            g.drawText("MIDI  {", midiRect.withTrimmedTop(6.0f).withHeight(18.0f).withTrimmedLeft(8.0f), juce::Justification::centredLeft, false);

            // Center Matrix Layout
            const float labelGutterW = 60.0f;
            const float actionGutterW = 44.0f;
            const float notesAreaW = tableRect.getWidth() - labelGutterW - actionGutterW - 8.0f;
            const float colW = notesAreaW / 12.0f;
            const float tableStartX = tableRect.getX() + labelGutterW;

            // Row Labels
            g.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff7a7d8c));
            g.drawText("Cents", juce::Rectangle<float>(tableRect.getX() + 8.0f, tableRect.getY() + 30.0f, labelGutterW - 12.0f, 20.0f), juce::Justification::centredLeft, false);
            g.drawText("Bypass", juce::Rectangle<float>(tableRect.getX() + 8.0f, tableRect.getY() + 58.0f, labelGutterW - 12.0f, 22.0f), juce::Justification::centredLeft, false);
            g.drawText("Remove", juce::Rectangle<float>(tableRect.getX() + 8.0f, tableRect.getY() + 88.0f, labelGutterW - 12.0f, 22.0f), juce::Justification::centredLeft, false);

            // Note Column Headers
            g.setFont(juce::Font(juce::FontOptions(11.5f).withStyle("Bold")));
            const auto noteNames = ToyTuneParameters::getRootNoteNames();
            for (int i = 0; i < 12; ++i)
            {
                const float colX = tableStartX + static_cast<float>(i) * colW;
                g.setColour(juce::Colour(0xffd5d8e2));
                g.drawText(noteNames[i], juce::Rectangle<float>(colX, tableRect.getY() + 8.0f, colW, 16.0f), juce::Justification::centred, false);
            }
        }
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        const int W = bounds.getWidth();
        const int headerH = 32;

        if (!isOpen)
        {
            // Closed State (Image 2): Single centered tab pill [ Advanced ︽ ]
            const int btnW = 114;
            const int btnH = 24;
            const int btnY = (headerH - btnH) / 2;
            advancedToggleBtn.setBounds((W - btnW) / 2, btnY, btnW, btnH);
            return;
        }

        // Expanded State (Image 3 & 4): Segmented bar [ Vibrato ] | [ Advanced ︾ ] | [ Scale ]
        const int tabBtnW = 84;
        const int toggleBtnW = 108;
        const int btnH = 24;
        const int btnY = (headerH - btnH) / 2;
        const int totalHeaderW = tabBtnW + 4 + toggleBtnW + 4 + tabBtnW;
        const int startX = (W - totalHeaderW) / 2;

        vibratoTabBtn.setBounds(startX, btnY, tabBtnW, btnH);
        advancedToggleBtn.setBounds(startX + tabBtnW + 4, btnY, toggleBtnW, btnH);
        scaleTabBtn.setBounds(startX + tabBtnW + 4 + toggleBtnW + 4, btnY, tabBtnW, btnH);

        auto contentArea = bounds.withTrimmedTop(headerH + 4).reduced(12, 6);

        if (activeTab == 0)
        {
            // Vibrato View Layout (Image 4)
            const int arrowW = 32;
            const int availableW = contentArea.getWidth() - arrowW - 16;
            const int createW = static_cast<int>(availableW * 0.58f);
            const int modW = availableW - createW;

            auto createRect = contentArea.removeFromLeft(createW);
            contentArea.removeFromLeft(arrowW + 16);
            auto modRect = contentArea.removeFromLeft(modW);

            // Create Vibrato Panel:
            // 2x2 Shape Grid on left
            const int shapeStartX = createRect.getX() + 16;
            const int shapeStartY = createRect.getY() + 46;
            const int shapeW = 52;
            const int shapeH = 24;
            const int shapeGap = 4;

            // Row 1: Off, Sine ~
            shapeButtons[0]->setBounds(shapeStartX, shapeStartY, shapeW, shapeH);
            shapeButtons[1]->setBounds(shapeStartX + shapeW + shapeGap, shapeStartY, shapeW, shapeH);
            // Row 2: Saw ⩘, Square ⎍
            shapeButtons[2]->setBounds(shapeStartX, shapeStartY + shapeH + shapeGap, shapeW, shapeH);
            shapeButtons[3]->setBounds(shapeStartX + shapeW + shapeGap, shapeStartY + shapeH + shapeGap, shapeW, shapeH);

            // 4 LFO Knobs on right
            const int lfoKnobsX = shapeStartX + (shapeW * 2) + shapeGap + 12;
            const int lfoKnobsW = createRect.getRight() - lfoKnobsX - 12;
            const int knobW = lfoKnobsW / 4;
            const int knobY = createRect.getY() + 28;
            const int knobH = std::max(60, createRect.getBottom() - knobY - 6);

            rateKnob.setBounds(lfoKnobsX, knobY, knobW, knobH);
            delayKnob.setBounds(lfoKnobsX + knobW, knobY, knobW, knobH);
            onsetKnob.setBounds(lfoKnobsX + knobW * 2, knobY, knobW, knobH);
            varKnob.setBounds(lfoKnobsX + knobW * 3, knobY, knobW, knobH);

            // Modulate Panel: 3 Knobs
            const int modKnobW = (modRect.getWidth() - 24) / 3;
            const int modKnobX = modRect.getX() + 12;

            pitchModKnob.setBounds(modKnobX, knobY, modKnobW, knobH);
            ampModKnob.setBounds(modKnobX + modKnobW, knobY, modKnobW, knobH);
            formantModKnob.setBounds(modKnobX + modKnobW * 2, knobY, modKnobW, knobH);
        }
        else
        {
            // Scale View Layout (Image 3)
            const int setW = 88;
            const int midiW = 94;
            const int tableW = contentArea.getWidth() - setW - midiW - 16;

            auto setRect = contentArea.removeFromLeft(setW);
            contentArea.removeFromLeft(8);
            auto tableRect = contentArea.removeFromLeft(tableW);
            contentArea.removeFromLeft(8);
            auto midiRect = contentArea;

            // Set Buttons (Major, Minor, All)
            const int setBtnH = 26;
            const int setBtnGap = 6;
            int setBtnY = setRect.getY() + 32;
            majorSetBtn.setBounds(setRect.getX() + 6, setBtnY, setW - 12, setBtnH);
            setBtnY += setBtnH + setBtnGap;
            minorSetBtn.setBounds(setRect.getX() + 6, setBtnY, setW - 12, setBtnH);
            setBtnY += setBtnH + setBtnGap;
            allSetBtn.setBounds(setRect.getX() + 6, setBtnY, setW - 12, setBtnH);

            // MIDI Buttons (To Notes, Learn Scale, All Octaves)
            int midiBtnY = midiRect.getY() + 32;
            midiToNotesBtn.setBounds(midiRect.getX() + 6, midiBtnY, midiW - 12, setBtnH);
            midiBtnY += setBtnH + setBtnGap;
            midiLearnScaleBtn.setBounds(midiRect.getX() + 6, midiBtnY, midiW - 12, setBtnH);
            midiBtnY += setBtnH + setBtnGap;
            midiAllOctavesBtn.setBounds(midiRect.getX() + 6, midiBtnY, midiW - 12, setBtnH);

            // 12 Note Columns in Matrix
            const int labelGutterW = 60;
            const int actionGutterW = 46;
            const int notesAreaW = tableRect.getWidth() - labelGutterW - actionGutterW - 8;
            const int colW = notesAreaW / 12;
            const int tableStartX = tableRect.getX() + labelGutterW;

            const int centsY = tableRect.getY() + 30;
            const int centsH = 20;
            const int bypY = centsY + centsH + 7;
            const int bypH = 24;
            const int remY = bypY + bypH + 6;
            const int remH = 24;

            for (int i = 0; i < 12; ++i)
            {
                const int colX = tableStartX + i * colW;
                centsCells[i]->setBounds(colX + 2, centsY, colW - 4, centsH);
                bypButtons[i]->setBounds(colX + 2, bypY, colW - 4, bypH);
                remButtons[i]->setBounds(colX + 2, remY, colW - 4, remH);
            }

            // Quick All buttons on the right of each row
            const int allX = tableStartX + 12 * colW + 4;
            zeroCentsBtn.setBounds(allX, centsY, actionGutterW - 4, centsH);
            bypAllBtn.setBounds(allX, bypY, actionGutterW - 4, bypH);
            remAllBtn.setBounds(allX, remY, actionGutterW - 4, remH);

            // Ignore Vibrato Button centered below matrix
            const int ignoreVibW = 126;
            const int ignoreVibH = 22;
            ignoreVibBtn.setBounds(tableStartX + (notesAreaW - ignoreVibW) / 2, remY + remH + 6, ignoreVibW, ignoreVibH);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    ScaleEngine& scaleEngine;

    bool isOpen { false };
    int activeTab { 1 }; // Default to Scale tab (Index 1)
    bool ignoreVibratoActive { false };

    // Header Controls
    juce::TextButton vibratoTabBtn;
    juce::TextButton advancedToggleBtn;
    juce::TextButton scaleTabBtn;

    // Scale Tab Components (Image 3)
    juce::TextButton majorSetBtn;
    juce::TextButton minorSetBtn;
    juce::TextButton allSetBtn;

    std::array<std::unique_ptr<NoteCentsCell>, 12> centsCells;
    std::array<std::unique_ptr<ScaleNoteButton>, 12> bypButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 12> bypAttachments;
    std::array<std::unique_ptr<ScaleNoteButton>, 12> remButtons;

    juce::TextButton zeroCentsBtn;
    juce::TextButton bypAllBtn;
    juce::TextButton remAllBtn;
    juce::TextButton ignoreVibBtn;

    juce::TextButton midiToNotesBtn;
    juce::TextButton midiLearnScaleBtn;
    juce::TextButton midiAllOctavesBtn;

    // Vibrato Tab Components (Image 4)
    std::array<std::unique_ptr<juce::TextButton>, 4> shapeButtons;
    RotarySliderWithLabel rateKnob;
    RotarySliderWithLabel delayKnob;
    RotarySliderWithLabel onsetKnob;
    RotarySliderWithLabel varKnob;
    RotarySliderWithLabel pitchModKnob;
    RotarySliderWithLabel ampModKnob;
    RotarySliderWithLabel formantModKnob;

    void updateHeaderUI()
    {
        const juce::Colour activeBg(0xffffffff);
        const juce::Colour inactiveBg(0xff141518);
        const juce::Colour activeText(0xff0a0a0d);
        const juce::Colour inactiveText(0xff8a8d9a);

        if (!isOpen)
        {
            // Closed State (Image 2): Only centered Advanced ︽ pill
            vibratoTabBtn.setVisible(false);
            scaleTabBtn.setVisible(false);
            advancedToggleBtn.setVisible(true);

            advancedToggleBtn.setButtonText("Advanced " + juce::String::charToString(0x25B2));
            advancedToggleBtn.setColour(juce::TextButton::buttonColourId, inactiveBg);
            advancedToggleBtn.setColour(juce::TextButton::textColourOffId, inactiveText);
        }
        else
        {
            // Open State (Image 3 & 4): [ Vibrato ] | [ Advanced ︾ ] | [ Scale ]
            vibratoTabBtn.setVisible(true);
            scaleTabBtn.setVisible(true);
            advancedToggleBtn.setVisible(true);

            advancedToggleBtn.setButtonText("Advanced " + juce::String::charToString(0x25BC));
            advancedToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff22242b));
            advancedToggleBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffffff));

            vibratoTabBtn.setColour(juce::TextButton::buttonColourId, (activeTab == 0) ? activeBg : inactiveBg);
            vibratoTabBtn.setColour(juce::TextButton::textColourOnId, (activeTab == 0) ? activeText : inactiveText);
            vibratoTabBtn.setColour(juce::TextButton::textColourOffId, (activeTab == 0) ? activeText : inactiveText);

            scaleTabBtn.setColour(juce::TextButton::buttonColourId, (activeTab == 1) ? activeBg : inactiveBg);
            scaleTabBtn.setColour(juce::TextButton::textColourOnId, (activeTab == 1) ? activeText : inactiveText);
            scaleTabBtn.setColour(juce::TextButton::textColourOffId, (activeTab == 1) ? activeText : inactiveText);
        }
    }

    void updateVisibility()
    {
        const bool showVibrato = isOpen && (activeTab == 0);
        const bool showScale   = isOpen && (activeTab == 1);

        // Header controls visibility
        vibratoTabBtn.setVisible(isOpen);
        scaleTabBtn.setVisible(isOpen);
        advancedToggleBtn.setVisible(true);

        // Vibrato Controls
        for (auto& btn : shapeButtons)
            if (btn) btn->setVisible(showVibrato);

        rateKnob.setVisible(showVibrato);
        delayKnob.setVisible(showVibrato);
        onsetKnob.setVisible(showVibrato);
        varKnob.setVisible(showVibrato);
        pitchModKnob.setVisible(showVibrato);
        ampModKnob.setVisible(showVibrato);
        formantModKnob.setVisible(showVibrato);

        // Scale Controls
        majorSetBtn.setVisible(showScale);
        minorSetBtn.setVisible(showScale);
        allSetBtn.setVisible(showScale);

        for (int i = 0; i < 12; ++i)
        {
            if (centsCells[i]) centsCells[i]->setVisible(showScale);
            if (bypButtons[i]) bypButtons[i]->setVisible(showScale);
            if (remButtons[i]) remButtons[i]->setVisible(showScale);
        }

        zeroCentsBtn.setVisible(showScale);
        bypAllBtn.setVisible(showScale);
        remAllBtn.setVisible(showScale);
        ignoreVibBtn.setVisible(showScale);

        midiToNotesBtn.setVisible(showScale);
        midiLearnScaleBtn.setVisible(showScale);
        midiAllOctavesBtn.setVisible(showScale);
    }

    void applyQuickSet(int setIndex)
    {
        // 0 = Major, 1 = Minor, 2 = All
        scaleEngine.applyQuickSet(setIndex);
        const auto mask = scaleEngine.getActiveMask();
        for (int i = 0; i < 12; ++i)
        {
            if (auto* param = apvts.getParameter(ParameterIDs::noteMask(i)))
                param->setValueNotifyingHost(mask[i] ? 1.0f : 0.0f);
        }
        updateRemButtonsUI();
    }

    void toggleAllBypass()
    {
        bool anyOn = false;
        for (int i = 0; i < 12; ++i)
        {
            if (auto* param = apvts.getRawParameterValue(ParameterIDs::noteBypass(i)))
            {
                if (param->load() > 0.5f) { anyOn = true; break; }
            }
        }
        const float targetVal = anyOn ? 0.0f : 1.0f;
        for (int i = 0; i < 12; ++i)
        {
            if (auto* param = apvts.getParameter(ParameterIDs::noteBypass(i)))
                param->setValueNotifyingHost(targetVal);
        }
    }

    void toggleAllRemove()
    {
        bool allActive = true;
        for (int i = 0; i < 12; ++i)
        {
            if (auto* param = apvts.getRawParameterValue(ParameterIDs::noteMask(i)))
            {
                if (param->load() < 0.5f) { allActive = false; break; }
            }
        }
        const float targetVal = allActive ? 0.0f : 1.0f;
        const int rootIdx = scaleEngine.getRootNote();

        for (int i = 0; i < 12; ++i)
        {
            if (auto* param = apvts.getParameter(ParameterIDs::noteMask(i)))
            {
                if (allActive && i == rootIdx)
                    param->setValueNotifyingHost(1.0f); // Always preserve root note
                else
                    param->setValueNotifyingHost(targetVal);
            }
        }
        updateRemButtonsUI();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedDrawerComponent)
};

} // namespace toytune
