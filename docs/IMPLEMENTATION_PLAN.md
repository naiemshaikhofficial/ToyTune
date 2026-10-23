# ToyTune — Implementation Plan
**Developer:** Producer Toy  
**Product:** ToyTune Real-Time Vocal Pitch Correction Plugin (VST3 / Standalone)  
**Framework:** JUCE 8 (C++17) & CMake  

---

## 1. Objectives & Quality Standards
- Deliver an original, production-grade vocal tuning plugin combining high-precision fundamental frequency (F0) tracking, real-time musical pitch correction with flexible response curves (Natural, Modern, Hard Tune, Robot), scalable scale/key masking, formant-aware processing, and a distinct charcoal/black luxury Producer Toy interface.
- Complete working DSP code: real pitch detection, real pitch shifting, zero dummy meters, and zero nonfunctional knobs.
- Strictly adhere to real-time audio thread safety: no allocations, no locks, bounded CPU, deterministic latency reporting.

---

## 2. Execution Phases

### Phase 1: Environment & Project Foundation
- **Tasks:**
  1. Configure `CMakeLists.txt` targeting VST3 and Standalone formats using JUCE 8 (`juce_add_plugin`, `juce_add_binary_data`).
  2. Implement `PluginProcessor` and `PluginParameters` with `juce::AudioProcessorValueTreeState`.
  3. Ensure clean compilation on Windows x64 (MSVC 2022).
  4. Verify clean audio passthrough and latency reporting.
- **Verification:** CMake configure, Release build compilation, and parameter tree initialization.

### Phase 2: Fundamental Frequency (F0) Detection Engine
- **Tasks:**
  1. Implement `PitchDetector` utilizing Cumulative Mean Normalized Difference (CMNDF / YIN-inspired) with sub-sample parabolic interpolation.
  2. Add vocal range bounds (Low Bass ~55 Hz to High Soprano ~1050 Hz).
  3. Integrate confidence thresholding, voiced/unvoiced silence gating, and octave-jump suppression.
  4. Implement lock-free telemetry (`AudioTelemetry`) transferring detected F0, confidence, and RMS levels to the UI thread safely.
- **Verification:** Unit tests with synthetic test tones (110 Hz, 220 Hz, 440 Hz, 880 Hz) across varied block sizes and sample rates.

### Phase 3: Musical Scale & Target Selection Engine
- **Tasks:**
  1. Implement `ScaleEngine` with support for all 12 root notes, scale types (Chromatic, Major, Natural Minor, Harmonic Minor, Melodic Minor, Pentatonic Major, Pentatonic Minor, Blues), and custom 12-semitone note masks.
  2. Implement reference pitch tuning (A4 = 415 Hz to 465 Hz, default 440 Hz).
  3. Map incoming F0 into MIDI fractional note, find nearest active target scale note, and calculate exact cents deviation.
  4. Implement Pitch Tolerance and Flex Tune deadband filters.
- **Verification:** Unit tests verifying target note mapping across all scale degrees and custom note masks.

### Phase 4: Pitch Shifting & Correction Engine
- **Tasks:**
  1. Implement real-time `PitchShifter` using synchronized dual-head variable delay lines with Hann window overlap-add (OLA) or time-domain pitch transposition.
  2. Implement `CorrectionSmoother` for Retune Speed (0 ms hard snap up to 400 ms smooth portamento), Humanize (vibrato preservation / micro-deviation preservation), and Strength (0% to 100%).
  3. Implement correction character modes:
     - **Natural:** Gradual retune curves preserving expressive vibrato and gentle centering.
     - **Modern:** Quick musical snap with smoothed note boundaries (commercial pop standard).
     - **Hard Tune:** Instantaneous snapping (0 ms retune, 100% strength) creating the signature modern vocal effect.
     - **Robot:** Locked fundamental quantization without drift.
  4. Implement stereo phase-linked processing (mono detection driving identical pitch ratios on L/R channels).
- **Verification:** Unit tests with detuned sine waves measuring output frequency shift toward target pitch.

### Phase 5: Formant Processing & Audio Polishing
- **Tasks:**
  1. Implement `FormantFilter` utilizing spectral tilt / formant preservation filters to counter the chipmunk / unnatural deepening effect when pitching vocals up or down.
  2. Formant Shift control (-12 to +12 semitones) and Formant Preserve switch.
  3. Dry/Wet mix and Input/Output gain stages with smooth ramping.
- **Verification:** Audio continuity tests, denormal checks, and latency verification.

### Phase 6: UI & Visual Monitoring System
- **Tasks:**
  1. Implement custom `ToyTuneLookAndFeel` adhering strictly to Producer Toy's charcoal/black aesthetic (`#0c0c0e`, `#151518`, `#202024`, brushed silver `#d4d4d8`).
  2. Design original UI layout:
     - Header: Producer Toy charcoal logo, ToyTune wordmark, preset manager, bypass, settings.
     - Central Pitch Lane: Real-time scrolling pitch trace, detected pitch, target note grid, cents deviation, note label.
     - Tuning Controls: Retune Speed, Humanize, Flex Tune, Correction Strength, Tuning Mode selector.
     - Keyboard / Scale Strip: 12-key interactive piano keyboard for interactive note masking, Root selector, Scale selector.
     - Utility Strip: Input/Output RMS meters, Vocal Range, Formant controls, Mix, Gain, Latency display.
  3. Connect all controls to APVTS via `SliderAttachment`, `ComboBoxAttachment`, and `ButtonAttachment`.
- **Verification:** Verify that all UI knobs, buttons, and meters are fully operational and synchronized with DAW parameters.

### Phase 7: Preset System, Tests & Validation
- **Tasks:**
  1. Implement `PresetManager` with factory presets: *Natural Vocal*, *Modern Pop*, *Fast Pop Tune*, *Hard Tune*, *Robot Vocal*, *Subtle Correction*, *Manual Scale*.
  2. Support saving/loading custom XML presets to user directory.
  3. Compile standalone application and VST3 plugin in Release mode.
  4. Run automated test suite (`PitchDetectionTests`, `ScaleEngineTests`, `PitchCorrectionTests`, `ParameterTests`).
  5. Document architecture, DSP mathematics, build instructions, and benchmark results.
