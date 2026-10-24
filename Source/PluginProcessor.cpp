#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace toytune
{

ToyTuneAudioProcessor::ToyTuneAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", ToyTuneParameters::createParameterLayout())
{
    presetManager = std::make_unique<PresetManager>(apvts);
    updateScaleEngineFromParameters();
}

ToyTuneAudioProcessor::~ToyTuneAudioProcessor() = default;

const juce::String ToyTuneAudioProcessor::getName() const
{
    return "ToyTune";
}

bool ToyTuneAudioProcessor::acceptsMidi() const
{
    return true;
}

bool ToyTuneAudioProcessor::producesMidi() const
{
    return false;
}

bool ToyTuneAudioProcessor::isMidiEffect() const
{
    return false;
}

double ToyTuneAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ToyTuneAudioProcessor::getNumPrograms()
{
    return static_cast<int>(presetManager->getFactoryPresets().size());
}

int ToyTuneAudioProcessor::getCurrentProgram()
{
    return presetManager->getCurrentPresetIndex();
}

void ToyTuneAudioProcessor::setCurrentProgram(int index)
{
    presetManager->applyPreset(index);
}

const juce::String ToyTuneAudioProcessor::getProgramName(int index)
{
    auto names = presetManager->getPresetNames();
    if (index >= 0 && index < names.size())
        return names[index];
    return {};
}

void ToyTuneAudioProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/)
{
}

void ToyTuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
    updateScaleEngineFromParameters();

    // Report algorithmic latency to host DAW for PDC
    setLatencySamples(engine.getLatencySamples());
}

void ToyTuneAudioProcessor::releaseResources()
{
    engine.reset();
}

bool ToyTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void ToyTuneAudioProcessor::updateScaleEngineFromParameters()
{
    const int rootIdx = static_cast<int>(apvts.getRawParameterValue(ParameterIDs::rootNote)->load());
    const int scaleIdx = static_cast<int>(apvts.getRawParameterValue(ParameterIDs::scaleType)->load());
    const float refHz = apvts.getRawParameterValue(ParameterIDs::referencePitch)->load();

    scaleEngine.setRootNote(rootIdx);
    scaleEngine.setScaleType(scaleIdx);
    scaleEngine.setReferencePitch(refHz);

    // Update 12 custom note masks, bypass states, and cents detune
    for (int i = 0; i < 12; ++i)
    {
        const bool active = apvts.getRawParameterValue(ParameterIDs::noteMask(i))->load() > 0.5f;
        scaleEngine.setCustomNoteActive(i, active);

        if (auto* bypParam = apvts.getRawParameterValue(ParameterIDs::noteBypass(i)))
            scaleEngine.setBypassSemitone(i, bypParam->load() > 0.5f);

        if (auto* centsParam = apvts.getRawParameterValue(ParameterIDs::noteCents(i)))
            scaleEngine.setCentsDetune(i, centsParam->load());
    }
}

void ToyTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updateScaleEngineFromParameters();
    engine.processMidi(midiMessages);

    // Read real-time parameters
    const float inGain = apvts.getRawParameterValue(ParameterIDs::inputGain)->load();
    const float outGain = apvts.getRawParameterValue(ParameterIDs::outputGain)->load();
    const float retuneSpeed = apvts.getRawParameterValue(ParameterIDs::retuneSpeed)->load();
    const float strength = apvts.getRawParameterValue(ParameterIDs::correctionStrength)->load();
    const float tolerance = apvts.getRawParameterValue(ParameterIDs::pitchTolerance)->load();
    const float humanize = apvts.getRawParameterValue(ParameterIDs::humanize)->load();
    const float flexTune = apvts.getRawParameterValue(ParameterIDs::flexTune)->load();
    const float vibratoPreserve = apvts.getRawParameterValue(ParameterIDs::vibratoPreserve)->load();
    const auto tuningMode = static_cast<TuningModeType>(static_cast<int>(apvts.getRawParameterValue(ParameterIDs::tuningMode)->load()));
    const auto vocalRange = static_cast<VocalRangeType>(static_cast<int>(apvts.getRawParameterValue(ParameterIDs::vocalRange)->load()));
    const float refHz = apvts.getRawParameterValue(ParameterIDs::referencePitch)->load();
    const bool formantPreserve = apvts.getRawParameterValue(ParameterIDs::formantPreserve)->load() > 0.5f;
    const float formantShift = apvts.getRawParameterValue(ParameterIDs::formantShift)->load();
    const bool lowLatency = apvts.getRawParameterValue(ParameterIDs::lowLatencyMode)->load() > 0.5f;
    const float mix = apvts.getRawParameterValue(ParameterIDs::dryWetMix)->load();
    const bool bypassed = apvts.getRawParameterValue(ParameterIDs::bypass)->load() > 0.5f;

    // Advanced Vibrato parameters
    auto* vibShapeVal = apvts.getRawParameterValue(ParameterIDs::advVibShape);
    auto* vibRateVal = apvts.getRawParameterValue(ParameterIDs::advVibRate);
    auto* vibDelayVal = apvts.getRawParameterValue(ParameterIDs::advVibDelay);
    auto* vibOnsetVal = apvts.getRawParameterValue(ParameterIDs::advVibOnset);
    auto* vibVarVal = apvts.getRawParameterValue(ParameterIDs::advVibVariation);
    auto* vibPitchVal = apvts.getRawParameterValue(ParameterIDs::advVibPitch);
    auto* vibAmpVal = apvts.getRawParameterValue(ParameterIDs::advVibAmp);
    auto* vibFormantVal = apvts.getRawParameterValue(ParameterIDs::advVibFormant);

    const auto advVibShape = (vibShapeVal != nullptr) ? static_cast<VibratoShape>(static_cast<int>(vibShapeVal->load())) : VibratoShape::Off;
    const float advVibRate = (vibRateVal != nullptr) ? vibRateVal->load() : 5.5f;
    const float advVibDelay = (vibDelayVal != nullptr) ? vibDelayVal->load() : 400.0f;
    const float advVibOnset = (vibOnsetVal != nullptr) ? vibOnsetVal->load() : 400.0f;
    const float advVibVar = (vibVarVal != nullptr) ? vibVarVal->load() : 15.0f;
    const float advVibPitch = (vibPitchVal != nullptr) ? vibPitchVal->load() : 0.0f;
    const float advVibAmp = (vibAmpVal != nullptr) ? vibAmpVal->load() : 0.0f;
    const float advVibFormant = (vibFormantVal != nullptr) ? vibFormantVal->load() : 0.0f;

    engine.setLowLatencyMode(lowLatency);
    const int currentLatency = engine.getLatencySamples();
    if (getLatencySamples() != currentLatency)
        setLatencySamples(currentLatency);

    engine.process(buffer,
                   scaleEngine,
                   telemetry,
                   inGain,
                   outGain,
                   retuneSpeed,
                   strength,
                   tolerance,
                   humanize,
                   flexTune,
                   vibratoPreserve,
                   tuningMode,
                   vocalRange,
                   refHz,
                   formantPreserve,
                   formantShift,
                   mix,
                   bypassed,
                   advVibShape,
                   advVibRate,
                   advVibDelay,
                   advVibOnset,
                   advVibVar,
                   advVibPitch,
                   advVibAmp,
                   advVibFormant);
}

bool ToyTuneAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ToyTuneAudioProcessor::createEditor()
{
    return new ToyTuneAudioProcessorEditor(*this);
}

void ToyTuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ToyTuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
            updateScaleEngineFromParameters();
        }
    }
}

} // namespace toytune

// JUCE Plugin Creation Entrypoint
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new toytune::ToyTuneAudioProcessor();
}
