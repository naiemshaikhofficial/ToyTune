cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# Producer Toy — ToyTune Real-Time Vocal Pitch Correction Plugin
# Professional VST3 / Standalone Audio Plugin Build
# ==============================================================================
project(ToyTune VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    message(STATUS "[ToyTune] Target Platform: Windows x64 (MSVC ABI)")
    if(MSVC)
        add_compile_options(/utf-8 /W4 /permissive-)
        # Disable specific noise warnings if needed
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    endif()
endif()

# Locate JUCE 8 Framework
set(LOCAL_JUCE_PATH "D:/Website/ProducerToy-Plugins/ProVerb/build/_deps/juce-src")

if(EXISTS "${LOCAL_JUCE_PATH}/CMakeLists.txt")
    message(STATUS "[ToyTune] Using existing local JUCE installation: ${LOCAL_JUCE_PATH}")
    add_subdirectory("${LOCAL_JUCE_PATH}" juce_build)
else()
    message(STATUS "[ToyTune] Fetching JUCE 8.0.4 from official GitHub repository...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# Binary Data for Embedded Logos & Assets
juce_add_binary_data(ToyTuneResources
    SOURCES
        assets/branding/producer-toy-charcoal-logo.png
        assets/branding/toytune-wordmark.png
)

# ToyTune Plugin Target (VST3 and Standalone)
juce_add_plugin(ToyTune
    COMPANY_NAME "Producer Toy"
    BUNDLE_ID "com.ProducerToy.ToyTune"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE PrTy
    PLUGIN_CODE TyTn
    FORMATS VST3 Standalone
    PRODUCT_NAME "ToyTune"
    DESCRIPTION "Professional Real-Time Vocal Pitch Correction Plugin"
)

# Source files
target_sources(ToyTune
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/Parameters/ToyTuneParameters.cpp
        Source/Parameters/ToyTuneParameters.h
        Source/DSP/CircularBuffer.h
        Source/DSP/PitchShifter.cpp
        Source/DSP/PitchShifter.h
        Source/DSP/FormantFilter.h
        Source/DSP/CorrectionSmoother.h
        Source/DSP/AudioTelemetry.h
        Source/PitchDetection/PitchDetector.cpp
        Source/PitchDetection/PitchDetector.h
        Source/PitchCorrection/ScaleEngine.cpp
        Source/PitchCorrection/ScaleEngine.h
        Source/PitchCorrection/TargetNoteSelector.h
        Source/PitchCorrection/PitchCorrectionEngine.h
        Source/Presets/PresetManager.cpp
        Source/Presets/PresetManager.h
        Source/UI/ToyTuneLookAndFeel.cpp
        Source/UI/ToyTuneLookAndFeel.h
        Source/UI/PitchHistoryComponent.cpp
        Source/UI/PitchHistoryComponent.h
        Source/UI/PitchLaneComponent.h
        Source/UI/RotarySliderWithLabel.h
        Source/UI/KeyboardScaleSelector.h
        Source/UI/LevelMeterComponent.h
        Source/UI/HeaderBarComponent.h
)

target_include_directories(ToyTune
    PRIVATE
        Source
        Source/Parameters
        Source/DSP
        Source/PitchDetection
        Source/PitchCorrection
        Source/Presets
        Source/UI
)

target_compile_definitions(ToyTune
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ToyTune
    PRIVATE
        ToyTuneResources
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_audio_devices
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_data_structures
        juce::juce_events
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Automated Test Executable
add_executable(ToyTuneTests
    Tests/PitchDetectionTests.cpp
    Tests/ScaleEngineTests.cpp
    Tests/PitchCorrectionTests.cpp
    Tests/ParameterTests.cpp
    Source/Parameters/ToyTuneParameters.cpp
    Source/PitchDetection/PitchDetector.cpp
    Source/PitchCorrection/ScaleEngine.cpp
    Source/DSP/PitchShifter.cpp
)

target_include_directories(ToyTuneTests
    PRIVATE
        Source
        Source/Parameters
        Source/DSP
        Source/PitchDetection
        Source/PitchCorrection
        Source/Presets
        Source/UI
)

target_link_libraries(ToyTuneTests
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
)
