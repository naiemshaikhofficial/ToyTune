#include "PresetManager.h"
#include "../Parameters/ToyTuneParameters.h"

namespace toytune
{

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts)
    : state(apvts)
{
    initFactoryPresets();
}

void PresetManager::initFactoryPresets()
{
    factoryPresets.clear();

    // 0. Default (Clean State matching Auto-Tune Default)
    factoryPresets.push_back({
        "Default",
        0.0f,   // retuneSpeed = 0 (No auto-tune applied)
        50.0f,  // noteTransition = 50%
        100.0f, // strength
        0.0f,   // tolerance = 0
        0.0f,   // humanize = 0
        0.0f,   // flexTune = 0
        0.0f,   // vibratoPreserve = 0
        1,      // Modern mode
        0,      // C
        0,      // Chromatic
        true,   // formantPreserve
        0.0f    // formantShift = 0
    });

    // 1. Modern Pop
    factoryPresets.push_back({
        "Modern Pop",
        85.0f,  // retuneSpeed (Fast, modern)
        65.0f,  // noteTransition (Natural legato glide)
        100.0f, // strength
        5.0f,   // tolerance
        10.0f,  // humanize
        15.0f,  // flexTune
        0.0f,   // vibratoPreserve
        1,      // Modern mode
        0,      // C
        1,      // Major
        true,   // formantPreserve
        0.0f
    });

    // 2. Fast Pop Tune
    factoryPresets.push_back({
        "Fast Pop Tune",
        95.0f,  // retuneSpeed (Very fast snap)
        40.0f,  // noteTransition
        100.0f, // strength
        0.0f,   // tolerance
        0.0f,   // humanize
        5.0f,   // flexTune
        0.0f,   // vibratoPreserve
        1,      // Modern mode
        0,      // C
        1,      // Major
        true,
        0.0f
    });

    // 3. Hard Tune (T-Pain / Travis Scott style)
    factoryPresets.push_back({
        "Hard Tune",
        100.0f, // retuneSpeed (Instant maximum hard snap)
        0.0f,   // noteTransition (Instant note jump)
        100.0f, // strength
        0.0f,   // tolerance
        0.0f,   // humanize
        0.0f,   // flexTune
        0.0f,   // vibratoPreserve
        0,      // Classic mode
        0,      // C
        2,      // Minor
        true,
        0.0f
    });

    // 4. Robot Vocal
    factoryPresets.push_back({
        "Robot Vocal",
        100.0f, // retuneSpeed (100% hard snap)
        0.0f,   // noteTransition (Instant note jump)
        100.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        3,      // Robot mode
        0,
        0,
        false,  // formantPreserve off gives distinct robotic resonance
        0.0f
    });

    // 5. Natural Vocal
    factoryPresets.push_back({
        "Natural Vocal",
        50.0f,  // retuneSpeed (Smooth, melodious)
        75.0f,  // noteTransition (Fluid natural slides)
        80.0f,  // strength
        15.0f,  // tolerance
        35.0f,  // humanize
        40.0f,  // flexTune
        0.0f,   // vibratoPreserve
        1,      // Modern mode
        0,      // C
        0,      // Chromatic
        true,   // formantPreserve
        0.0f    // formantShift
    });

    // 6. Subtle Correction
    factoryPresets.push_back({
        "Subtle Correction",
        25.0f,  // retuneSpeed (Gentle, subtle)
        85.0f,  // noteTransition
        60.0f,
        25.0f,
        60.0f,
        50.0f,
        0.0f,
        1,      // Modern mode
        0,
        0,      // Chromatic
        true,
        0.0f
    });

    // 7. Manual Scale
    factoryPresets.push_back({
        "Manual Scale",
        20.0f,
        50.0f,  // noteTransition
        100.0f,
        0.0f,
        10.0f,
        10.0f,
        50.0f,
        1,
        0,
        8,      // Custom mask
        true,
        0.0f
    });
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& p : factoryPresets)
        names.add(p.name);
    return names;
}

void PresetManager::setParameterValue(const char* paramId, float value)
{
    if (auto* param = state.getParameter(paramId))
    {
        const auto range = param->getNormalisableRange();
        param->setValueNotifyingHost(range.convertTo0to1(value));
    }
}

void PresetManager::applyPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(factoryPresets.size()))
        return;

    currentPresetIndex = index;
    const auto& p = factoryPresets[static_cast<size_t>(index)];

    setParameterValue(ParameterIDs::retuneSpeed, p.retuneSpeed);
    setParameterValue(ParameterIDs::noteTransition, p.noteTransition);
    setParameterValue(ParameterIDs::correctionStrength, p.strength);
    setParameterValue(ParameterIDs::pitchTolerance, p.tolerance);
    setParameterValue(ParameterIDs::humanize, p.humanize);
    setParameterValue(ParameterIDs::flexTune, p.flexTune);
    setParameterValue(ParameterIDs::vibratoPreserve, p.vibratoPreserve);
    setParameterValue(ParameterIDs::tuningMode, static_cast<float>(p.tuningMode));
    setParameterValue(ParameterIDs::rootNote, static_cast<float>(p.rootNote));
    setParameterValue(ParameterIDs::scaleType, static_cast<float>(p.scaleType));
    setParameterValue(ParameterIDs::formantPreserve, p.formantPreserve ? 1.0f : 0.0f);
    setParameterValue(ParameterIDs::formantShift, p.formantShift);
}

void PresetManager::applyPreset(const juce::String& name)
{
    for (size_t i = 0; i < factoryPresets.size(); ++i)
    {
        if (factoryPresets[i].name.equalsIgnoreCase(name))
        {
            applyPreset(static_cast<int>(i));
            return;
        }
    }
}

bool PresetManager::saveUserPreset(const juce::String& name)
{
    auto userDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("ProducerToy")
                       .getChildFile("ToyTune")
                       .getChildFile("Presets");
    userDir.createDirectory();

    auto targetFile = userDir.getChildFile(name + ".toypreset");
    auto xml = state.copyState().createXml();
    if (xml != nullptr)
    {
        return xml->writeTo(targetFile);
    }
    return false;
}

bool PresetManager::loadUserPreset(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr && xml->hasTagName(state.state.getType()))
    {
        state.replaceState(juce::ValueTree::fromXml(*xml));
        return true;
    }
    return false;
}

} // namespace toytune
