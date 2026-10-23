#include "PluginEditor.h"

namespace toytune
{

ToyTuneAudioProcessorEditor::ToyTuneAudioProcessorEditor(ToyTuneAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      headerBar(p.getAPVTS(), p.getPresetManager()),
      pitchLane(p.getTelemetry()),
      keyboardSelector(p.getAPVTS()),
      retuneSpeedKnob("RETUNE", "ms"),
      correctionStrengthKnob("STRENGTH", "%"),
      pitchToleranceKnob("TOLERANCE", "ct"),
      humanizeKnob("HUMANIZE", "%"),
      flexTuneKnob("FLEX TUNE", "%"),
      vibratoPreserveKnob("VIBRATO", "%"),
      inMeter("IN"),
      outMeter("OUT"),
      inputGainKnob("IN GAIN", "dB"),
      outputGainKnob("OUT GAIN", "dB"),
      mixKnob("DRY/WET", "%"),
      formantShiftKnob("FORMANT", "st")
{
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);

    // 1. Header Bar
    addAndMakeVisible(headerBar);

    // 2. Central Pitch Workspace
    addAndMakeVisible(pitchLane);

    // 3. Key & Scale Section
    scaleGroup.setText("KEY & SCALE");
    scaleGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff222228));
    scaleGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff8a8a95));
    addAndMakeVisible(scaleGroup);

    rootLabel.setText("Root", juce::dontSendNotification);
    rootLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    rootLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9a9aa5));
    addAndMakeVisible(rootLabel);

    rootKeyBox.addItemList(ToyTuneParameters::getRootNoteNames(), 1);
    rootKeyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::rootNote, rootKeyBox);
    addAndMakeVisible(rootKeyBox);

    scaleLabel.setText("Scale", juce::dontSendNotification);
    scaleLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    scaleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9a9aa5));
    addAndMakeVisible(scaleLabel);

    scaleTypeBox.addItemList(ToyTuneParameters::getScaleTypeNames(), 1);
    scaleTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::scaleType, scaleTypeBox);
    addAndMakeVisible(scaleTypeBox);

    refPitchLabel.setText("A4 Ref", juce::dontSendNotification);
    refPitchLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    refPitchLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9a9aa5));
    addAndMakeVisible(refPitchLabel);

    refPitchSlider.setSliderStyle(juce::Slider::LinearBar);
    refPitchSlider.setTextValueSuffix(" Hz");
    refPitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::referencePitch, refPitchSlider);
    addAndMakeVisible(refPitchSlider);

    addAndMakeVisible(keyboardSelector);

    // 4. Dedicated Tuning Controls Section
    tuningGroup.setText("TUNING DYNAMICS");
    tuningGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff222228));
    tuningGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff8a8a95));
    addAndMakeVisible(tuningGroup);

    retuneSpeedKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::retuneSpeed);
    addAndMakeVisible(retuneSpeedKnob);

    correctionStrengthKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::correctionStrength);
    addAndMakeVisible(correctionStrengthKnob);

    pitchToleranceKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::pitchTolerance);
    addAndMakeVisible(pitchToleranceKnob);

    humanizeKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::humanize);
    addAndMakeVisible(humanizeKnob);

    flexTuneKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::flexTune);
    addAndMakeVisible(flexTuneKnob);

    vibratoPreserveKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::vibratoPreserve);
    addAndMakeVisible(vibratoPreserveKnob);

    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9a9aa5));
    addAndMakeVisible(modeLabel);

    tuningModeBox.addItemList(ToyTuneParameters::getTuningModeNames(), 1);
    tuningModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::tuningMode, tuningModeBox);
    addAndMakeVisible(tuningModeBox);

    // 5. Bottom Utility Strip
    utilityGroup.setText("MASTER & VOICE UTILITY");
    utilityGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff222228));
    utilityGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff8a8a95));
    addAndMakeVisible(utilityGroup);

    addAndMakeVisible(inMeter);
    addAndMakeVisible(outMeter);

    inputGainKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::inputGain);
    addAndMakeVisible(inputGainKnob);

    outputGainKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::outputGain);
    addAndMakeVisible(outputGainKnob);

    mixKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::dryWetMix);
    addAndMakeVisible(mixKnob);

    vocalRangeLabel.setText("Vocal Range", juce::dontSendNotification);
    vocalRangeLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    vocalRangeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9a9aa5));
    addAndMakeVisible(vocalRangeLabel);

    vocalRangeBox.addItemList(ToyTuneParameters::getVocalRangeNames(), 1);
    vocalRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::vocalRange, vocalRangeBox);
    addAndMakeVisible(vocalRangeBox);

    formantPreserveBtn.setButtonText("Preserve Formants");
    formantPreserveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::formantPreserve, formantPreserveBtn);
    addAndMakeVisible(formantPreserveBtn);

    formantShiftKnob.attachToParameter(audioProcessor.getAPVTS(), ParameterIDs::formantShift);
    addAndMakeVisible(formantShiftKnob);

    lowLatencyBtn.setButtonText("Low Latency");
    lowLatencyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::lowLatencyMode, lowLatencyBtn);
    addAndMakeVisible(lowLatencyBtn);

    latencyDisplay.setFont(juce::Font(juce::FontOptions(10.5f)));
    latencyDisplay.setColour(juce::Label::textColourId, juce::Colour(0xff6e6e78));
    latencyDisplay.setText("Latency: 512 smp (11.6 ms)", juce::dontSendNotification);
    addAndMakeVisible(latencyDisplay);

    setSize(920, 640);
    startTimerHz(30); // 30 FPS smooth telemetry update
}

ToyTuneAudioProcessorEditor::~ToyTuneAudioProcessorEditor()
{
    stopTimer();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void ToyTuneAudioProcessorEditor::timerCallback()
{
    auto& telemetry = audioProcessor.getTelemetry();

    // 1. Update Pitch Lane & History
    pitchLane.updateTelemetry();

    // 2. Update Meters
    inMeter.setLevel(telemetry.inputRmsDb.load(std::memory_order_relaxed));
    outMeter.setLevel(telemetry.outputRmsDb.load(std::memory_order_relaxed));

    // 3. Highlight detected note on piano keyboard
    const float detMidi = telemetry.detectedMidiNote.load(std::memory_order_relaxed);
    const bool isVoiced = telemetry.isVoiced.load(std::memory_order_relaxed);
    if (isVoiced && detMidi > 10.0f)
    {
        const int semitone = (static_cast<int>(std::round(detMidi)) % 12 + 12) % 12;
        keyboardSelector.setActiveNoteDetected(semitone);
    }
    else
    {
        keyboardSelector.setActiveNoteDetected(-1);
    }

    keyboardSelector.updateScaleHighlights(audioProcessor.getScaleEngine().getActiveMask());

    // 4. Update Latency readout
    const int latencySamples = audioProcessor.getLatencySamples();
    const double sr = audioProcessor.getSampleRate() > 1000.0 ? audioProcessor.getSampleRate() : 44100.0;
    const double latencyMs = (static_cast<double>(latencySamples) / sr) * 1000.0;
    latencyDisplay.setText("PDC Latency: " + juce::String(latencySamples) + " smp (" + juce::String(latencyMs, 1) + " ms)",
                           juce::dontSendNotification);
}

void ToyTuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Charcoal Luxury Background
    g.fillAll(ToyTuneLookAndFeel::backgroundDark);
}

void ToyTuneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // 1. Header (50px height)
    headerBar.setBounds(area.removeFromTop(50));

    // Margins
    area.reduce(12, 10);

    // 2. Bottom Utility Strip (115px height)
    auto utilityArea = area.removeFromBottom(115);
    utilityGroup.setBounds(utilityArea);
    auto uContent = utilityArea.reduced(10, 16);

    // In & Out Meters
    inMeter.setBounds(uContent.removeFromLeft(24));
    uContent.removeFromLeft(6);
    outMeter.setBounds(uContent.removeFromLeft(24));
    uContent.removeFromLeft(16);

    // Input Gain, Output Gain, Dry/Wet Mix knobs
    inputGainKnob.setBounds(uContent.removeFromLeft(68));
    uContent.removeFromLeft(6);
    outputGainKnob.setBounds(uContent.removeFromLeft(68));
    uContent.removeFromLeft(6);
    mixKnob.setBounds(uContent.removeFromLeft(68));
    uContent.removeFromLeft(20);

    // Vocal Range
    auto rangeArea = uContent.removeFromLeft(140);
    vocalRangeLabel.setBounds(rangeArea.removeFromTop(18));
    vocalRangeBox.setBounds(rangeArea.removeFromTop(24));
    uContent.removeFromLeft(16);

    // Formant Controls
    auto formantArea = uContent.removeFromLeft(170);
    formantShiftKnob.setBounds(formantArea.removeFromLeft(68));
    formantArea.removeFromLeft(8);
    formantPreserveBtn.setBounds(formantArea.removeFromTop(24));
    uContent.removeFromLeft(16);

    // Latency controls
    lowLatencyBtn.setBounds(uContent.removeFromTop(24));
    latencyDisplay.setBounds(uContent.removeFromTop(20));

    area.removeFromBottom(10);

    // 3. Key & Scale Section (bottom-middle, 75px height)
    auto scaleArea = area.removeFromBottom(75);
    scaleGroup.setBounds(scaleArea);
    auto sContent = scaleArea.reduced(10, 14);

    // Root
    auto rootArea = sContent.removeFromLeft(70);
    rootLabel.setBounds(rootArea.removeFromTop(16));
    rootKeyBox.setBounds(rootArea.removeFromTop(24));
    sContent.removeFromLeft(8);

    // Scale
    auto scaleSelectArea = sContent.removeFromLeft(130);
    scaleLabel.setBounds(scaleSelectArea.removeFromTop(16));
    scaleTypeBox.setBounds(scaleSelectArea.removeFromTop(24));
    sContent.removeFromLeft(8);

    // A4 Ref Pitch
    auto refArea = sContent.removeFromLeft(80);
    refPitchLabel.setBounds(refArea.removeFromTop(16));
    refPitchSlider.setBounds(refArea.removeFromTop(24));
    sContent.removeFromLeft(16);

    // Interactive Piano Keyboard
    keyboardSelector.setBounds(sContent);

    area.removeFromBottom(10);

    // 4. Middle Split: Left = Dedicated Tuning Controls (310px wide), Right = Central Pitch Workspace
    auto tuningArea = area.removeFromRight(330);
    tuningGroup.setBounds(tuningArea);
    auto tContent = tuningArea.reduced(10, 14);

    // Mode dropdown at top of tuning panel
    auto modeArea = tContent.removeFromTop(44);
    modeLabel.setBounds(modeArea.removeFromTop(16));
    tuningModeBox.setBounds(modeArea.removeFromTop(24));

    tContent.removeFromTop(8);

    // 2 rows of 3 knobs
    const int knobW = 96;
    const int knobH = 76;

    auto row1 = tContent.removeFromTop(knobH);
    retuneSpeedKnob.setBounds(row1.removeFromLeft(knobW));
    row1.removeFromLeft(8);
    correctionStrengthKnob.setBounds(row1.removeFromLeft(knobW));
    row1.removeFromLeft(8);
    pitchToleranceKnob.setBounds(row1.removeFromLeft(knobW));

    tContent.removeFromTop(10);

    auto row2 = tContent.removeFromTop(knobH);
    humanizeKnob.setBounds(row2.removeFromLeft(knobW));
    row2.removeFromLeft(8);
    flexTuneKnob.setBounds(row2.removeFromLeft(knobW));
    row2.removeFromLeft(8);
    vibratoPreserveKnob.setBounds(row2.removeFromLeft(knobW));

    area.removeFromRight(10);

    // Central Pitch Workspace occupies remaining area
    pitchLane.setBounds(area);
}

} // namespace toytune
