#include "PluginEditor.h"
#include "Parameters/ToyTuneParameters.h"

namespace toytune
{

ToyTuneAudioProcessorEditor::ToyTuneAudioProcessorEditor(ToyTuneAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      headerBar(audioProcessor.getAPVTS(), audioProcessor.getPresetManager()),
      retuneSpeedKnob("Retune Speed", "", false, false),
      noteTransitionKnob("Transition", "", false, false),
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

    noteTransitionKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::noteTransition);
    addAndMakeVisible(noteTransitionKnob);

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

    // Dynamic UI states based on Tuning Mode (Modern vs Classic)
    if (tuningModeSelector != nullptr)
    {
        tuningModeSelector->onSelectionChanged = [this](int /*segIndex*/)
        {
            updateTuningModeUI();
        };
    }
    updateTuningModeUI();

    // Expanded Widescreen Plugin Dimensions (ProVerb-inspired spacious, non-congested dashboard)
    setSize(1360, 860);
    setResizable(true, true);
    setResizeLimits(1000, 640, 2200, 1400);

    // 60 FPS ultra-smooth audio telemetry and pitch needle updates
    startTimerHz(60);
}

ToyTuneAudioProcessorEditor::~ToyTuneAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ToyTuneAudioProcessorEditor::updateTuningModeUI()
{
    if (auto* param = audioProcessor.getAPVTS().getRawParameterValue(ParameterIDs::tuningMode))
    {
        const int modeVal = static_cast<int>(std::round(param->load()));
        // In ToyTune / Auto-Tune:
        // modeVal == 0 is Classic (Natural)
        // modeVal == 1 is Modern
        // In Classic mode, Flex Tune and Formant Shift are disabled (Auto-Tune 5 architecture)
        const bool isClassic = (modeVal == 0);
        const bool shouldEnable = !isClassic;

        if (flexTuneKnob.isEnabled() != shouldEnable)
        {
            flexTuneKnob.setEnabled(shouldEnable);
            formantKnob.setEnabled(shouldEnable);
        }
    }
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

    updateTuningModeUI();
}

void ToyTuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // 1. Overall Dark Luxury Matte Background
    juce::ColourGradient bgGrad(juce::Colour(0xff0d0e11), bounds.getCentreX(), bounds.getY(),
                                juce::Colour(0xff07080a), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 12.0f);

    // Subtle outer metallic chassis rim
    g.setColour(juce::Colour(0xff1e2027));
    g.drawRoundedRectangle(bounds, 12.0f, 1.5f);

    // 2. Angled Ventilation Grip Ridges on Left and Right Chassis Margins
    g.setColour(juce::Colour(0x30ffffff));
    const float gripStartX_L = bounds.getX() + juce::jlimit(16.0f, 26.0f, bounds.getWidth() * 0.018f);
    const float gripStartX_R = bounds.getRight() - juce::jlimit(16.0f, 26.0f, bounds.getWidth() * 0.018f);

    // Upper Grip Lines (Centered vertically beside Left Tuning Card and Humanize Card)
    const float upperCenterY = static_cast<float>(leftTuningCardBounds.getCentreY());
    for (int i = 0; i < 5; ++i)
    {
        const float y = upperCenterY - 24.0f + static_cast<float>(i) * 12.0f;
        // Left angled vents
        g.drawLine(gripStartX_L - 8.0f, y + 10.0f, gripStartX_L + 2.0f, y - 2.0f, 2.2f);
        // Right angled vents
        g.drawLine(gripStartX_R - 2.0f, y + 10.0f, gripStartX_R + 8.0f, y - 2.0f, 2.2f);
    }

    // Lower Grip Lines (Centered vertically beside Bottom Shelf)
    const float lowerCenterY = static_cast<float>(bottomShelfBounds.getCentreY());
    for (int i = 0; i < 4; ++i)
    {
        const float y = lowerCenterY - 18.0f + static_cast<float>(i) * 12.0f;
        g.drawLine(gripStartX_L - 8.0f, y + 10.0f, gripStartX_L + 2.0f, y - 2.0f, 2.2f);
        g.drawLine(gripStartX_R - 2.0f, y + 10.0f, gripStartX_R + 8.0f, y - 2.0f, 2.2f);
    }

    // 2b. Left Tuning Card Background (Retune Speed on Top, Note Transition on Bottom)
    const auto tuningCardRect = leftTuningCardBounds.toFloat();
    g.setColour(juce::Colour(0xff101114));
    g.fillRoundedRectangle(tuningCardRect, 12.0f);
    g.setColour(juce::Colour(0xff1e2027));
    g.drawRoundedRectangle(tuningCardRect, 12.0f, 1.2f);

    // 3. Bottom Control Shelf Background Card
    const auto shelfRect = bottomShelfBounds.toFloat();
    g.setColour(juce::Colour(0xff101114));
    g.fillRoundedRectangle(shelfRect, 12.0f);
    g.setColour(juce::Colour(0xff1e2027));
    g.drawRoundedRectangle(shelfRect, 12.0f, 1.2f);
}

void ToyTuneAudioProcessorEditor::resized()
{
    const auto bounds = getLocalBounds();
    const int W = bounds.getWidth();
    const int H = bounds.getHeight();

    // 1. Top Header Bar
    const int headerH = juce::jlimit(58, 76, static_cast<int>(H * 0.082f));
    headerBar.setBounds(0, 0, W, headerH);

    // Dynamic horizontal margin for spacious chassis feel
    const int sideMargin = juce::jlimit(36, 68, static_cast<int>(W * 0.045f));

    // 2. Upper Selectors Row
    const int upperY = headerH + juce::jlimit(10, 20, static_cast<int>(H * 0.018f));
    const int selectorH = juce::jlimit(30, 36, static_cast<int>(H * 0.042f));
    const int labelH = 16;
    const int upperH = labelH + 4 + selectorH;

    // Left: Input Type
    const int inputTypeW = juce::jlimit(190, 230, static_cast<int>(W * 0.165f));
    inputTypeLabel.setBounds(sideMargin, upperY, inputTypeW, labelH);
    if (inputTypeSelector != nullptr)
        inputTypeSelector->setBounds(sideMargin, upperY + labelH + 4, inputTypeW, selectorH);

    // Center: Key & Scale
    const int rootW = juce::jlimit(64, 80, static_cast<int>(W * 0.058f));
    const int scaleW = juce::jlimit(98, 124, static_cast<int>(W * 0.09f));
    const int comboGap = 8;
    const int keyScaleW = rootW + comboGap + scaleW;
    const int keyScaleX = bounds.getCentreX() - (keyScaleW / 2);
    keyScaleLabel.setBounds(keyScaleX, upperY, keyScaleW, labelH);
    rootKeyBox.setBounds(keyScaleX, upperY + labelH + 4, rootW, selectorH);
    scaleTypeBox.setBounds(keyScaleX + rootW + comboGap, upperY + labelH + 4, scaleW, selectorH);

    // Right: Tuning Mode
    const int modeW = juce::jlimit(180, 220, static_cast<int>(W * 0.16f));
    const int modeX = bounds.getRight() - sideMargin - modeW;
    tuningModeLabel.setBounds(modeX, upperY, modeW, labelH);
    if (tuningModeSelector != nullptr)
        tuningModeSelector->setBounds(modeX, upperY + labelH + 4, modeW, selectorH);

    // 5. Bottom Control Section (Compute bottom first so middle can fill cleanly)
    const int shelfX = sideMargin;
    const int shelfW = W - 2 * sideMargin;
    const int shelfH = juce::jlimit(190, 250, static_cast<int>(H * 0.285f));
    const int bottomMargin = juce::jlimit(12, 22, static_cast<int>(H * 0.022f));
    const int shelfY = H - shelfH - bottomMargin;
    bottomShelfBounds = juce::Rectangle<int>(shelfX, shelfY, shelfW, shelfH);

    // 4. Chromatic Keyboard Bar (Above bottom shelf)
    const int kbH = juce::jlimit(36, 46, static_cast<int>(H * 0.052f));
    const int kbGap = juce::jlimit(10, 18, static_cast<int>(H * 0.018f));
    const int kbY = shelfY - kbH - kbGap;
    const int kbMargin = sideMargin + juce::jlimit(14, 32, static_cast<int>(W * 0.022f));
    keyboardSelector.setBounds(kbMargin, kbY, W - 2 * kbMargin, kbH);

    // 3. Middle Section (Between Upper row and Keyboard Bar)
    const int midY = upperY + upperH + juce::jlimit(10, 20, static_cast<int>(H * 0.018f));
    const int midH = std::max(180, kbY - kbGap - midY);
    const int cardW = juce::jlimit(142, 200, static_cast<int>(W * 0.142f));

    // Left Card: Retune Speed on Top, Note Transition on Bottom
    leftTuningCardBounds = juce::Rectangle<int>(sideMargin, midY, cardW, midH);
    const int cardInnerPad = juce::jlimit(4, 8, static_cast<int>(cardW * 0.04f));
    const int cardContentW = cardW - 2 * cardInnerPad;
    const int singleKnobH = (midH - 3 * cardInnerPad) / 2;

    retuneSpeedKnob.setBounds(sideMargin + cardInnerPad,
                              midY + cardInnerPad,
                              cardContentW,
                              singleKnobH);

    noteTransitionKnob.setBounds(sideMargin + cardInnerPad,
                                 midY + cardInnerPad * 2 + singleKnobH,
                                 cardContentW,
                                 singleKnobH);

    // Right Card: Humanize
    humanizeKnob.setBounds(modeX + modeW - cardW, midY, cardW, midH);

    // Center: Circular Pitch Display
    const int displayPad = juce::jlimit(10, 18, static_cast<int>(W * 0.012f));
    const int displayX = sideMargin + cardW + displayPad;
    const int displayW = (modeX + modeW - cardW) - displayPad - displayX;
    centralPitchDisplay.setBounds(displayX, midY, displayW, midH);

    // Layout inside Bottom Shelf
    const int shelfInnerPad = juce::jlimit(14, 26, static_cast<int>(shelfW * 0.02f));
    const int knobW = juce::jlimit(82, 112, static_cast<int>(shelfW * 0.086f));
    const int knobH = shelfH - juce::jlimit(28, 46, static_cast<int>(shelfH * 0.18f));
    const int knobY = shelfY + (shelfH - knobH) / 2 + 5;
    const int knobGap = juce::jlimit(8, 14, static_cast<int>(shelfW * 0.009f));

    // Left 3 Knobs (Flex Tune, Vibrato, Formant)
    flexTuneKnob.setBounds(shelfX + shelfInnerPad, knobY, knobW, knobH);
    vibratoKnob.setBounds(shelfX + shelfInnerPad + knobW + knobGap, knobY, knobW, knobH);
    formantKnob.setBounds(shelfX + shelfInnerPad + (knobW + knobGap) * 2, knobY, knobW, knobH);

    // Right 2 Knobs (Mix, Output)
    const int rightKnobsTotalW = knobW * 2 + knobGap;
    const int mixX = shelfX + shelfW - shelfInnerPad - rightKnobsTotalW;
    mixKnob.setBounds(mixX, knobY, knobW, knobH);
    outputGainKnob.setBounds(mixX + knobW + knobGap, knobY, knobW, knobH);

    // Center Range Slider
    const int rangePad = juce::jlimit(16, 28, static_cast<int>(shelfW * 0.02f));
    const int rangeX = shelfX + shelfInnerPad + (knobW + knobGap) * 3 + rangePad;
    const int rangeW = mixX - rangePad - rangeX;
    const int rangeH = juce::jlimit(105, 140, static_cast<int>(shelfH * 0.58f));
    const int rangeY = shelfY + (shelfH - rangeH) / 2;
    rangeSlider.setBounds(rangeX, rangeY, rangeW, rangeH);
}

} // namespace toytune
