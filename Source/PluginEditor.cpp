#include "PluginEditor.h"
#include "Parameters/ToyTuneParameters.h"

namespace toytune
{

ToyTuneAudioProcessorEditor::ToyTuneAudioProcessorEditor(ToyTuneAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      headerBar(audioProcessor.getAPVTS(), audioProcessor.getPresetManager()),
      retuneSpeedKnob("Retune Speed", "", false, true),
      centralPitchDisplay(audioProcessor.getTelemetry()),
      humanizeKnob("Humanize", "", false, true),
      keyboardSelector(audioProcessor.getAPVTS()),
      flexTuneKnob("Flex Tune", "", false, false),
      vibratoKnob("Vibrato", "", false, false),
      formantKnob("Formant", "", false, false),
      mixKnob("Mix", "", false, false),
      outputGainKnob("Output", "dB", true, false)
{
    setLookAndFeel(&customLookAndFeel);

    // 1. Top Header Bar
    addAndMakeVisible(headerBar);

    // 2. Upper Selectors Row
    // Input Type Label & Selector (Auto, Male, Female)
    inputTypeLabel.setText("Input Type", juce::dontSendNotification);
    inputTypeLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    inputTypeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a8d9a));
    inputTypeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(inputTypeLabel);

    inputTypeSelector = std::make_unique<SegmentedPillComponent>(
        juce::StringArray { "Auto", "Male", "Female" },
        audioProcessor.getAPVTS(),
        ParameterIDs::vocalRange,
        std::vector<int> { 0, 1, 3 } // 0=Wide(Auto), 1=BassBaritone(Male), 3=AltoSoprano(Female)
    );
    addAndMakeVisible(*inputTypeSelector);

    // Key & Scale Label & ComboBoxes
    keyScaleLabel.setText("Key & Scale", juce::dontSendNotification);
    keyScaleLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    keyScaleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a8d9a));
    keyScaleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(keyScaleLabel);

    rootKeyBox.addItemList(ToyTuneParameters::getRootNoteNames(), 1);
    rootKeyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::rootNote, rootKeyBox);
    addAndMakeVisible(rootKeyBox);

    scaleTypeBox.addItemList(ToyTuneParameters::getScaleTypeNames(), 1);
    scaleTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::scaleType, scaleTypeBox);
    addAndMakeVisible(scaleTypeBox);

    // Tuning Mode Label & Selector (Modern, Classic)
    tuningModeLabel.setText("Tuning Mode", juce::dontSendNotification);
    tuningModeLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    tuningModeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a8d9a));
    tuningModeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(tuningModeLabel);

    tuningModeSelector = std::make_unique<SegmentedPillComponent>(
        juce::StringArray { "Modern", "Classic" },
        audioProcessor.getAPVTS(),
        ParameterIDs::tuningMode,
        std::vector<int> { 1, 0 } // 1=Modern, 0=Natural/Classic
    );
    addAndMakeVisible(*tuningModeSelector);

    // 3. Middle Section
    retuneSpeedKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::retuneSpeed);
    addAndMakeVisible(retuneSpeedKnob);

    addAndMakeVisible(centralPitchDisplay);

    humanizeKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::humanize);
    addAndMakeVisible(humanizeKnob);

    // 4. Chromatic Keyboard
    addAndMakeVisible(keyboardSelector);

    // 5. Bottom Section
    flexTuneKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::flexTune);
    addAndMakeVisible(flexTuneKnob);

    vibratoKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::vibratoPreserve);
    addAndMakeVisible(vibratoKnob);

    formantKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::formantShift);
    addAndMakeVisible(formantKnob);

    rangeSlider.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::pitchTolerance);
    addAndMakeVisible(rangeSlider);

    mixKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::dryWetMix);
    addAndMakeVisible(mixKnob);

    outputGainKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::outputGain);
    addAndMakeVisible(outputGainKnob);

    // Fixed aspect window matching reference: 1000 x 660
    setSize(1000, 660);

    // 60 FPS ultra-smooth audio telemetry and pitch needle updates
    startTimerHz(60);
}

ToyTuneAudioProcessorEditor::~ToyTuneAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ToyTuneAudioProcessorEditor::timerCallback()
{
    // Update real-time pitch analysis display
    centralPitchDisplay.updateTelemetry();

    // Update real-time active sung note on chromatic keyboard
    const bool voiced = audioProcessor.getTelemetry().isVoiced.load(std::memory_order_relaxed);
    const float targetMidi = audioProcessor.getTelemetry().targetMidiNote.load(std::memory_order_relaxed);
    const float detMidi = audioProcessor.getTelemetry().detectedMidiNote.load(std::memory_order_relaxed);

    int activeSemitone = -1;
    if (voiced && detMidi >= 20.0f)
    {
        activeSemitone = (static_cast<int>(std::round(targetMidi > 0.0f ? targetMidi : detMidi)) % 12 + 12) % 12;
    }
    keyboardSelector.setActiveNoteDetected(activeSemitone);

    // Keep segmented selectors synchronized if automation or presets changed them
    if (inputTypeSelector != nullptr)
        inputTypeSelector->updateFromParameter();

    if (tuningModeSelector != nullptr)
        tuningModeSelector->updateFromParameter();
}

void ToyTuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // 1. Overall Dark Luxury Matte Background with Cyber Vignette
    juce::ColourGradient bgGrad(juce::Colour(0xff0d0e12), bounds.getCentreX(), bounds.getY(),
                                juce::Colour(0xff060709), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 12.0f);

    // Outer Aerospace Machined Rim
    g.setColour(juce::Colour(0xff22242d));
    g.drawRoundedRectangle(bounds, 12.0f, 1.6f);

    // 2. Corner CNC Hex Bolts (Aerospace Hardware Fasteners)
    auto drawHexBolt = [&g](float bx, float by)
    {
        // Recessed circular counterbore
        g.setColour(juce::Colour(0xff090a0d));
        g.fillEllipse(bx - 6.0f, by - 6.0f, 12.0f, 12.0f);
        g.setColour(juce::Colour(0xff252732));
        g.drawEllipse(bx - 6.0f, by - 6.0f, 12.0f, 12.0f, 1.0f);

        // Hexagon bolt head
        juce::Path hex;
        for (int i = 0; i < 6; ++i)
        {
            const float a = static_cast<float>(i) * (juce::MathConstants<float>::pi / 3.0f);
            const float x = bx + std::cos(a) * 3.6f;
            const float y = by + std::sin(a) * 3.6f;
            if (i == 0) hex.startNewSubPath(x, y);
            else        hex.lineTo(x, y);
        }
        hex.closeSubPath();
        g.setColour(juce::Colour(0xff323540));
        g.fillPath(hex);
        g.setColour(juce::Colour(0x50ffffff));
        g.strokePath(hex, juce::PathStrokeType(0.8f));
    };

    drawHexBolt(18.0f, 18.0f);
    drawHexBolt(bounds.getRight() - 18.0f, 18.0f);
    drawHexBolt(18.0f, bounds.getBottom() - 18.0f);
    drawHexBolt(bounds.getRight() - 18.0f, bounds.getBottom() - 18.0f);

    // 3. Laser Hairline Guide Separators
    g.setColour(juce::Colour(0x18ffffff));
    g.drawHorizontalLine(57, 44.0f, bounds.getRight() - 44.0f);
    g.drawHorizontalLine(120, 44.0f, bounds.getRight() - 44.0f);
    g.drawHorizontalLine(422, 44.0f, bounds.getRight() - 44.0f);

    // 4. Angled Ventilation Grip Ridges on Left and Right Chassis Margins
    g.setColour(juce::Colour(0x35ffffff));
    const float gripStartX_L = bounds.getX() + 20.0f;
    const float gripStartX_R = bounds.getRight() - 20.0f;

    for (int i = 0; i < 5; ++i)
    {
        const float y = 220.0f + static_cast<float>(i) * 12.0f;
        // Left angled vents with depth shadow
        g.setColour(juce::Colour(0x20000000));
        g.drawLine(gripStartX_L - 8.0f, y + 11.0f, gripStartX_L + 2.0f, y - 1.0f, 2.6f);
        g.setColour(juce::Colour(0x35ffffff));
        g.drawLine(gripStartX_L - 8.0f, y + 10.0f, gripStartX_L + 2.0f, y - 2.0f, 1.8f);

        // Right angled vents
        g.setColour(juce::Colour(0x20000000));
        g.drawLine(gripStartX_R - 2.0f, y + 11.0f, gripStartX_R + 8.0f, y - 1.0f, 2.6f);
        g.setColour(juce::Colour(0x35ffffff));
        g.drawLine(gripStartX_R - 2.0f, y + 10.0f, gripStartX_R + 8.0f, y - 2.0f, 1.8f);
    }

    // Lower Grip Lines (Next to Bottom Shelf)
    for (int i = 0; i < 4; ++i)
    {
        const float y = 525.0f + static_cast<float>(i) * 12.0f;
        g.setColour(juce::Colour(0x35ffffff));
        g.drawLine(gripStartX_L - 8.0f, y + 10.0f, gripStartX_L + 2.0f, y - 2.0f, 1.8f);
        g.drawLine(gripStartX_R - 2.0f, y + 10.0f, gripStartX_R + 8.0f, y - 2.0f, 1.8f);
    }

    // 5. Bottom Control Shelf Background Card with Laser Machined Bezel
    const auto shelfRect = bottomShelfBounds.toFloat();
    g.setColour(juce::Colour(0xff101115));
    g.fillRoundedRectangle(shelfRect, 10.0f);
    g.setColour(juce::Colour(0xff22252e));
    g.drawRoundedRectangle(shelfRect, 10.0f, 1.2f);

    // Stenciled Technical Stencil Label on Bottom Shelf
    g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));
    g.setColour(juce::Colour(0xff555866));
    g.drawText("QUANTUM  PITCH  PROCESSING  RACK  //  MODEL  TT-01",
               static_cast<int>(shelfRect.getX() + 24.0f),
               static_cast<int>(shelfRect.getY() + 8.0f),
               320, 12, juce::Justification::centredLeft, false);

    // Status Armed LED indicator
    g.setColour(juce::Colour(0x40ffffff));
    g.fillEllipse(shelfRect.getRight() - 142.0f, shelfRect.getY() + 11.0f, 6.0f, 6.0f);
    g.setColour(juce::Colour(0xffffffff));
    g.fillEllipse(shelfRect.getRight() - 141.0f, shelfRect.getY() + 12.0f, 4.0f, 4.0f);

    g.setColour(juce::Colour(0xff8a8d9a));
    g.drawText("SYS: ARMED // 48kHz",
               static_cast<int>(shelfRect.getRight() - 130.0f),
               static_cast<int>(shelfRect.getY() + 8.0f),
               110, 12, juce::Justification::centredLeft, false);
}

void ToyTuneAudioProcessorEditor::resized()
{
    const auto bounds = getLocalBounds();

    // 1. Top Header Bar
    headerBar.setBounds(0, 0, bounds.getWidth(), 56);

    // 2. Upper Selectors Row (y=66 to 110)
    const int upperY = 66;

    // Left: Input Type
    inputTypeLabel.setBounds(44, upperY, 180, 16);
    if (inputTypeSelector != nullptr)
        inputTypeSelector->setBounds(44, upperY + 18, 190, 30);

    // Center: Key & Scale
    const int centerKeyScaleW = 168;
    const int centerKeyScaleX = bounds.getCentreX() - (centerKeyScaleW / 2);
    keyScaleLabel.setBounds(centerKeyScaleX, upperY, centerKeyScaleW, 16);
    rootKeyBox.setBounds(centerKeyScaleX, upperY + 18, 64, 30);
    scaleTypeBox.setBounds(centerKeyScaleX + 70, upperY + 18, 98, 30);

    // Right: Tuning Mode
    const int modeW = 180;
    const int modeX = bounds.getRight() - 44 - modeW;
    tuningModeLabel.setBounds(modeX, upperY, modeW, 16);
    if (tuningModeSelector != nullptr)
        tuningModeSelector->setBounds(modeX, upperY + 18, modeW, 30);

    // 3. Middle Section (y=128 to 366)
    const int midY = 128;
    const int cardW = 142;
    const int cardH = 228;

    // Left Card: Retune Speed
    retuneSpeedKnob.setBounds(44, midY, cardW, cardH);

    // Center: Circular Pitch Display
    const int displayX = 44 + cardW + 12;
    const int displayW = modeX - displayX - 12;
    centralPitchDisplay.setBounds(displayX, midY, displayW, cardH);

    // Right Card: Humanize
    humanizeKnob.setBounds(modeX + modeW - cardW, midY, cardW, cardH);

    // 4. Chromatic Keyboard Bar (y=376 to 414)
    keyboardSelector.setBounds(68, 376, bounds.getWidth() - 136, 36);

    // 5. Bottom Control Section (y=432 to 628)
    const int shelfX = 44;
    const int shelfW = bounds.getWidth() - 88;
    const int shelfY = 432;
    const int shelfH = 196;
    bottomShelfBounds = juce::Rectangle<int>(shelfX, shelfY, shelfW, shelfH);

    // Sub-group: Left 3 Knobs (Flex Tune, Vibrato, Formant)
    const int knobW = 82;
    const int knobH = 150;
    const int knobY = shelfY + 22;

    flexTuneKnob.setBounds(shelfX + 22, knobY, knobW, knobH);
    vibratoKnob.setBounds(shelfX + 22 + knobW + 10, knobY, knobW, knobH);
    formantKnob.setBounds(shelfX + 22 + (knobW + 10) * 2, knobY, knobW, knobH);

    // Sub-group: Center Pitch Correction Range Slider
    const int rangeX = shelfX + 22 + (knobW + 10) * 3 + 18;
    const int rightKnobsTotalW = (knobW + 12) * 2;
    const int rangeW = (shelfX + shelfW - rightKnobsTotalW - 22) - rangeX - 18;
    rangeSlider.setBounds(rangeX, shelfY + 34, rangeW, 110);

    // Sub-group: Right 2 Knobs (Mix, Output)
    const int mixX = shelfX + shelfW - rightKnobsTotalW - 14;
    mixKnob.setBounds(mixX, knobY, knobW, knobH);
    outputGainKnob.setBounds(mixX + knobW + 12, knobY, knobW, knobH);
}

} // namespace toytune
