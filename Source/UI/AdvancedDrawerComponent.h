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
 * Auto-Tune Pro-grade Advanced Drawer Component.
 * Supports collapsible slide-down drawer with two rich tabs:
 * 1. Vibrato View: 'Create Vibrato' LFO Generator (Shape: Off/Sine/Square/Saw, Rate, Delay, Onset Rate, Variation)
 *    and 'Modulate' Target Routing (Pitch in cents, Amplitude in %, Formant in %).
 * 2. Scale View: Quick Set (Major, Minor, All) + 12 Note Columns with microtonal Cents detuning (-50 to +50 ct),
 *    Bypass (BYP) toggles for passing blues bends/microtones, and Remove (REM) toggles.
 * Fully styled in ToyTune dark luxury monochrome aesthetic with glowing white indicators.
 */
class AdvancedDrawerComponent : public juce::Component
{
public:
    AdvancedDrawerComponent(juce::AudioProcessorValueTreeState& state, ScaleEngine& scaleEng)
        : apvts(state),
          scaleEngine(scaleEng),
          rateKnob("Rate", "Hz", true),
          delayKnob("Delay", "ms", false),
          onsetKnob("Onset", "ms", false),
          varKnob("Variation", "%", false),
          pitchModKnob("Pitch", "ct", false),
          ampModKnob("Amp", "%", false),
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

        advancedToggleBtn.setButtonText("Advanced ︽");
        advancedToggleBtn.onClick = [this]()
        {
            setDrawerOpen(!isOpen);
        };
        addAndMakeVisible(advancedToggleBtn);

        scaleTabBtn.setButtonText("Scale");
        scaleTabBtn.setClickingTogglesState(false);
        scaleTabBtn.onClick = [this]()
        {
            if (!isOpen)
                setDrawerOpen(true);
            setActiveTab(1);
        };
        addAndMakeVisible(scaleTabBtn);

        // 2. Vibrato Tab Components
        // Shape selector buttons: Off, Sine, Square, Saw
        static const char* shapeNames[] = { "Off", "~", "[_]", "/|" };
        for (int i = 0; i < 4; ++i)
        {
            shapeButtons[i] = std::make_unique<juce::TextButton>(shapeNames[i]);
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

        // Knobs attachments
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

        // 3. Scale Tab Components
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
        for (int i = 0; i < 12; ++i)
        {
            // Cents Detune Slider (-50 to +50 cents)
            centsSliders[i] = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::NoTextBox);
            centsSliders[i]->setRange(-50.0, 50.0, 1.0);
            centsSliders[i]->setDoubleClickReturnValue(true, 0.0);
            centsAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, ParameterIDs::noteCents(i), *centsSliders[i]);
            centsSliders[i]->onValueChange = [this]() { repaint(); };
            addAndMakeVisible(*centsSliders[i]);

            // Bypass Button (BYP)
            bypButtons[i] = std::make_unique<juce::TextButton>("BYP");
            bypButtons[i]->setClickingTogglesState(true);
            bypAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, ParameterIDs::noteBypass(i), *bypButtons[i]);
            addAndMakeVisible(*bypButtons[i]);

            // Remove Button (REM) - tied to noteMask (if true = active, if false = removed)
            remButtons[i] = std::make_unique<juce::TextButton>("REM");
            remButtons[i]->setClickingTogglesState(true);
            const int noteIdx = i;
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

        // Quick row buttons
        bypAllBtn.setButtonText("All");
        bypAllBtn.onClick = [this]() { toggleAllBypass(); };
        addAndMakeVisible(bypAllBtn);

        remAllBtn.setButtonText("All");
        remAllBtn.onClick = [this]() { toggleAllRemove(); };
        addAndMakeVisible(remAllBtn);

        zeroCentsBtn.setButtonText("Reset Cents");
        zeroCentsBtn.onClick = [this]()
        {
            for (int i = 0; i < 12; ++i)
            {
                if (auto* param = apvts.getParameter(ParameterIDs::noteCents(i)))
                    param->setValueNotifyingHost(param->convertTo0to1(0.0f));
            }
        };
        addAndMakeVisible(zeroCentsBtn);

        // Read initial state from APVTS
        if (auto* openVal = apvts.getRawParameterValue(ParameterIDs::advDrawerOpen))
            isOpen = openVal->load() > 0.5f;

        if (auto* tabVal = apvts.getRawParameterValue(ParameterIDs::advActiveTab))
            activeTab = static_cast<int>(std::round(tabVal->load()));

        updateHeaderUI();
        updateShapeButtonsUI();
        updateRemButtonsUI();
    }

    void setDrawerOpen(bool open)
    {
        if (isOpen != open)
        {
            isOpen = open;
            if (auto* param = apvts.getParameter(ParameterIDs::advDrawerOpen))
                param->setValueNotifyingHost(isOpen ? 1.0f : 0.0f);

            updateHeaderUI();
            if (onDrawerToggled)
                onDrawerToggled(isOpen);
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
        repaint();
    }

    int getActiveTab() const noexcept { return activeTab; }

    int getDesiredHeight() const noexcept
    {
        return isOpen ? 188 : 32;
    }

    std::function<void(bool)> onDrawerToggled;

    void updateRemButtonsUI()
    {
        for (int i = 0; i < 12; ++i)
        {
            if (auto* param = apvts.getRawParameterValue(ParameterIDs::noteMask(i)))
            {
                const bool isActive = param->load() > 0.5f;
                // REM button is highlighted if the note is REMOVED (!isActive)
                remButtons[i]->setToggleState(!isActive, juce::dontSendNotification);
            }
        }
    }

    void updateShapeButtonsUI()
    {
        int currentShape = 0;
        if (auto* param = apvts.getRawParameterValue(ParameterIDs::advVibShape))
            currentShape = static_cast<int>(std::round(param->load()));

        for (int i = 0; i < 4; ++i)
            shapeButtons[i]->setToggleState(i == currentShape, juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // 1. Drawer Container Background & Border
        g.setColour(juce::Colour(0xff0e0f12));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(juce::Colour(0xff1e2027));
        g.drawRoundedRectangle(bounds, 8.0f, 1.2f);

        // 2. Header Dividers
        const float headerH = 32.0f;
        g.setColour(juce::Colour(0xff181a20));
        g.drawLine(bounds.getX(), bounds.getY() + headerH, bounds.getRight(), bounds.getY() + headerH, 1.0f);

        if (!isOpen)
            return;

        // 3. Tab Specific Renderings
        const auto contentArea = bounds.withTrimmedTop(headerH + 4.0f).reduced(12.0f, 4.0f);

        if (activeTab == 0)
        {
            // Vibrato View Panels
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

            // Group Titles
            g.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff8a8d9a));
            g.drawText("CREATE VIBRATO", createRect.withTrimmedTop(6.0f).withHeight(16.0f), juce::Justification::centred, false);
            g.drawText("MODULATE", modRect.withTrimmedTop(6.0f).withHeight(16.0f), juce::Justification::centred, false);

            // Flow Arrow
            g.setColour(juce::Colour(0xff555968));
            g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
            g.drawText("-->", arrowRect, juce::Justification::centred, false);
        }
        else
        {
            // Scale View Panels
            const float setW = 80.0f;
            const float actionsW = 90.0f;
            const float tableW = contentArea.getWidth() - setW - actionsW - 16.0f;

            const auto setRect = juce::Rectangle<float>(contentArea.getX(), contentArea.getY(), setW, contentArea.getHeight());
            const auto tableRect = juce::Rectangle<float>(setRect.getRight() + 8.0f, contentArea.getY(), tableW, contentArea.getHeight());
            const auto actionsRect = juce::Rectangle<float>(tableRect.getRight() + 8.0f, contentArea.getY(), actionsW, contentArea.getHeight());

            g.setColour(juce::Colour(0xff121317));
            g.fillRoundedRectangle(setRect, 6.0f);
            g.fillRoundedRectangle(tableRect, 6.0f);
            g.fillRoundedRectangle(actionsRect, 6.0f);

            g.setColour(juce::Colour(0xff1c1e25));
            g.drawRoundedRectangle(setRect, 6.0f, 1.0f);
            g.drawRoundedRectangle(tableRect, 6.0f, 1.0f);
            g.drawRoundedRectangle(actionsRect, 6.0f, 1.0f);

            // Group Titles
            g.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff8a8d9a));
            g.drawText("SET", setRect.withTrimmedTop(6.0f).withHeight(16.0f), juce::Justification::centred, false);
            g.drawText("ACTIONS", actionsRect.withTrimmedTop(6.0f).withHeight(16.0f), juce::Justification::centred, false);

            // Table Column Headers & Row Labels
            const float colW = (tableRect.getWidth() - 36.0f) / 12.0f;
            g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
            
            for (int i = 0; i < 12; ++i)
            {
                const float colX = tableRect.getX() + 4.0f + static_cast<float>(i) * colW;
                const auto noteName = ToyTuneParameters::getRootNoteNames()[i];
                
                // Note Name Header
                g.setColour(juce::Colour(0xffd5d8e2));
                g.drawText(noteName, juce::Rectangle<float>(colX, tableRect.getY() + 4.0f, colW, 16.0f), juce::Justification::centred, false);

                // Cents value display
                const float centsVal = static_cast<float>(centsSliders[i]->getValue());
                g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Regular")));
                g.setColour((std::abs(centsVal) > 0.5f) ? juce::Colour(0xffffffff) : juce::Colour(0xff555968));
                const juce::String centsStr = (centsVal > 0 ? "+" : "") + juce::String(static_cast<int>(std::round(centsVal)));
                g.drawText(centsStr, juce::Rectangle<float>(colX, tableRect.getY() + 22.0f, colW, 14.0f), juce::Justification::centred, false);
            }

            // Row Labels on left of table
            g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
            g.setColour(juce::Colour(0xff777a88));
            g.drawText("Cents", juce::Rectangle<float>(tableRect.getX() + 4.0f, tableRect.getY() + 38.0f, 32.0f, 14.0f), juce::Justification::centredLeft, false);
        }
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        const int W = bounds.getWidth();
        const int headerH = 32;

        // 1. Header Bar: Center [ Advanced ︽/︾ ] with [ Vibrato ] on left and [ Scale ] on right
        const int toggleBtnW = 110;
        const int tabBtnW = 80;
        const int btnH = 24;
        const int btnY = (headerH - btnH) / 2;
        const int centerX = W / 2;

        advancedToggleBtn.setBounds(centerX - toggleBtnW / 2, btnY, toggleBtnW, btnH);
        vibratoTabBtn.setBounds(centerX - toggleBtnW / 2 - tabBtnW - 8, btnY, tabBtnW, btnH);
        scaleTabBtn.setBounds(centerX + toggleBtnW / 2 + 8, btnY, tabBtnW, btnH);

        updateVisibility();

        if (!isOpen)
            return;

        auto contentArea = bounds.withTrimmedTop(headerH + 4).reduced(12, 6);

        if (activeTab == 0)
        {
            // Vibrato View Layout
            const int arrowW = 32;
            const int availableW = contentArea.getWidth() - arrowW - 16;
            const int createW = static_cast<int>(availableW * 0.58f);
            const int modW = availableW - createW;

            auto createRect = contentArea.removeFromLeft(createW);
            contentArea.removeFromLeft(arrowW + 16); // Skip arrow gap
            auto modRect = contentArea.removeFromLeft(modW);

            // In Create Vibrato Card:
            // Top: 4 Shape Buttons
            const int shapeCardTop = createRect.getY() + 26;
            const int shapeW = 42;
            const int shapeH = 22;
            const int shapeGap = 4;
            const int totalShapesW = 4 * shapeW + 3 * shapeGap;
            int shapeStartX = createRect.getCentreX() - (totalShapesW / 2);

            for (int i = 0; i < 4; ++i)
            {
                shapeButtons[i]->setBounds(shapeStartX, shapeCardTop, shapeW, shapeH);
                shapeStartX += shapeW + shapeGap;
            }

            // Bottom: 4 Knobs (Rate, Delay, Onset, Variation)
            const int knobY = shapeCardTop + shapeH + 6;
            const int knobH = createRect.getBottom() - knobY - 4;
            const int knobW = (createRect.getWidth() - 20) / 4;
            int knobX = createRect.getX() + 10;

            rateKnob.setBounds(knobX, knobY, knobW, knobH);
            knobX += knobW;
            delayKnob.setBounds(knobX, knobY, knobW, knobH);
            knobX += knobW;
            onsetKnob.setBounds(knobX, knobY, knobW, knobH);
            knobX += knobW;
            varKnob.setBounds(knobX, knobY, knobW, knobH);

            // In Modulate Card: 3 Knobs (Pitch, Amp, Formant)
            const int modKnobY = modRect.getY() + 26;
            const int modKnobH = modRect.getBottom() - modKnobY - 4;
            const int modKnobW = (modRect.getWidth() - 16) / 3;
            int modKnobX = modRect.getX() + 8;

            pitchModKnob.setBounds(modKnobX, modKnobY, modKnobW, modKnobH);
            modKnobX += modKnobW;
            ampModKnob.setBounds(modKnobX, modKnobY, modKnobW, modKnobH);
            modKnobX += modKnobW;
            formantModKnob.setBounds(modKnobX, modKnobY, modKnobW, modKnobH);
        }
        else
        {
            // Scale View Layout
            const int setW = 80;
            const int actionsW = 90;
            const int tableW = contentArea.getWidth() - setW - actionsW - 16;

            auto setRect = contentArea.removeFromLeft(setW);
            contentArea.removeFromLeft(8);
            auto tableRect = contentArea.removeFromLeft(tableW);
            contentArea.removeFromLeft(8);
            auto actionsRect = contentArea;

            // Set Buttons (Major, Minor, All)
            const int setBtnH = 26;
            const int setBtnGap = 6;
            int setBtnY = setRect.getY() + 28;
            majorSetBtn.setBounds(setRect.getX() + 8, setBtnY, setW - 16, setBtnH);
            setBtnY += setBtnH + setBtnGap;
            minorSetBtn.setBounds(setRect.getX() + 8, setBtnY, setW - 16, setBtnH);
            setBtnY += setBtnH + setBtnGap;
            allSetBtn.setBounds(setRect.getX() + 8, setBtnY, setW - 16, setBtnH);

            // Actions Buttons
            int actBtnY = actionsRect.getY() + 32;
            zeroCentsBtn.setBounds(actionsRect.getX() + 6, actBtnY, actionsW - 12, 28);

            // Table 12 Columns
            const int colW = (tableRect.getWidth() - 36) / 12;
            const int tableStartX = tableRect.getX() + 4;

            const int sliderY = tableRect.getY() + 38;
            const int sliderH = 40;
            const int bypY = sliderY + sliderH + 4;
            const int bypH = 22;
            const int remY = bypY + bypH + 4;
            const int remH = 22;

            for (int i = 0; i < 12; ++i)
            {
                const int colX = tableStartX + i * colW;
                centsSliders[i]->setBounds(colX + 2, sliderY, colW - 4, sliderH);
                bypButtons[i]->setBounds(colX + 2, bypY, colW - 4, bypH);
                remButtons[i]->setBounds(colX + 2, remY, colW - 4, remH);
            }

            // Quick All buttons on the right of the table
            const int allX = tableStartX + 12 * colW + 4;
            bypAllBtn.setBounds(allX, bypY, 28, bypH);
            remAllBtn.setBounds(allX, remY, 28, remH);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    ScaleEngine& scaleEngine;

    bool isOpen { false };
    int activeTab { 0 }; // 0 = Vibrato, 1 = Scale

    // Header Controls
    juce::TextButton vibratoTabBtn;
    juce::TextButton advancedToggleBtn;
    juce::TextButton scaleTabBtn;

    // Vibrato Tab Components
    std::array<std::unique_ptr<juce::TextButton>, 4> shapeButtons;
    RotarySliderWithLabel rateKnob;
    RotarySliderWithLabel delayKnob;
    RotarySliderWithLabel onsetKnob;
    RotarySliderWithLabel varKnob;
    RotarySliderWithLabel pitchModKnob;
    RotarySliderWithLabel ampModKnob;
    RotarySliderWithLabel formantModKnob;

    // Scale Tab Components
    juce::TextButton majorSetBtn;
    juce::TextButton minorSetBtn;
    juce::TextButton allSetBtn;

    std::array<std::unique_ptr<juce::Slider>, 12> centsSliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 12> centsAttachments;
    std::array<std::unique_ptr<juce::TextButton>, 12> bypButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 12> bypAttachments;
    std::array<std::unique_ptr<juce::TextButton>, 12> remButtons;

    juce::TextButton bypAllBtn;
    juce::TextButton remAllBtn;
    juce::TextButton zeroCentsBtn;

    void updateHeaderUI()
    {
        advancedToggleBtn.setButtonText(isOpen ? "Advanced ︾" : "Advanced ︽");
        
        // Highlight active tab
        const juce::Colour activeBg(0xffffffff);
        const juce::Colour inactiveBg(0xff16171b);
        const juce::Colour activeText(0xff0a0a0d);
        const juce::Colour inactiveText(0xffa1a4b2);

        vibratoTabBtn.setColour(juce::TextButton::buttonColourId, (isOpen && activeTab == 0) ? activeBg : inactiveBg);
        vibratoTabBtn.setColour(juce::TextButton::textColourOnId, (isOpen && activeTab == 0) ? activeText : inactiveText);
        vibratoTabBtn.setColour(juce::TextButton::textColourOffId, (isOpen && activeTab == 0) ? activeText : inactiveText);

        scaleTabBtn.setColour(juce::TextButton::buttonColourId, (isOpen && activeTab == 1) ? activeBg : inactiveBg);
        scaleTabBtn.setColour(juce::TextButton::textColourOnId, (isOpen && activeTab == 1) ? activeText : inactiveText);
        scaleTabBtn.setColour(juce::TextButton::textColourOffId, (isOpen && activeTab == 1) ? activeText : inactiveText);

        advancedToggleBtn.setColour(juce::TextButton::buttonColourId, isOpen ? juce::Colour(0xff22242b) : inactiveBg);
        advancedToggleBtn.setColour(juce::TextButton::textColourOffId, isOpen ? juce::Colour(0xffffffff) : inactiveText);
    }

    void updateVisibility()
    {
        const bool showVibrato = isOpen && (activeTab == 0);
        const bool showScale = isOpen && (activeTab == 1);

        // Vibrato Controls
        for (auto& btn : shapeButtons)
            btn->setVisible(showVibrato);

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
            centsSliders[i]->setVisible(showScale);
            bypButtons[i]->setVisible(showScale);
            remButtons[i]->setVisible(showScale);
        }

        bypAllBtn.setVisible(showScale);
        remAllBtn.setVisible(showScale);
        zeroCentsBtn.setVisible(showScale);
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
        // Check if any is currently on
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
        // Toggle all note masks between all true and only root true
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
                    param->setValueNotifyingHost(1.0f); // Always keep root active
                else
                    param->setValueNotifyingHost(targetVal);
            }
        }
        updateRemButtonsUI();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedDrawerComponent)
};

} // namespace toytune
