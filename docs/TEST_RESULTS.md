# ToyTune — Automated DSP Test Results & Benchmark Report
**Developer:** Producer Toy  
**Product:** ToyTune Real-Time Vocal Pitch Correction  
**Date:** 2026-10-23  
**Platform:** Windows x64 (MSVC 19.44 / Release Mode)  
**Framework:** JUCE 8.0.4  

---

## 1. Executive Summary
The automated DSP verification suite `ToyTuneTests.exe` was executed directly against the compiled DSP and parameter engines. All unit and algorithmic benchmark tests completed with **100% pass rate**.

---

## 2. Test Execution Log

```
========================================
  TOYTUNE DSP & AUDIO ENGINE TEST SUITE 
========================================

=== Running Pitch Detection Tests ===
  [PitchDetector] Freq: 110 Hz | Detected: 109.913 Hz | Conf: 0.994666 | Error: 0.0793734% -> PASS
  [PitchDetector] Freq: 220 Hz | Detected: 220.069 Hz | Conf: 0.933968 | Error: 0.0312458% -> PASS
  [PitchDetector] Freq: 440 Hz | Detected: 440.113 Hz | Conf: 0.963770 | Error: 0.0257457% -> PASS
  [PitchDetector] Freq: 880 Hz | Detected: 880.326 Hz | Conf: 0.980684 | Error: 0.0370927% -> PASS
  [PitchDetector] Silence Gating: isVoiced = false -> PASS

=== Running Scale Engine Tests ===
  [ScaleEngine] Chromatic 61.35 -> Target: 61 (C#4) Dev: -34.9998 ct -> PASS
  [ScaleEngine] C Major on C# (61.1) -> Target: 62 (D4) -> PASS
  [ScaleEngine] Custom Mask Only A -> Target Note: 57 (A3) -> PASS

=== Running Pitch Correction Tests ===
  [PitchShifter] HQ Latency: 771 smp | LL Latency: 396 smp -> PASS
  [CorrectionSmoother] Hard Tune 40ct snap -> Ratio: 1.02337 (Expected: 1.02337) -> PASS
  [CorrectionSmoother] Natural Mode 50ms -> Initial Ratio: 1.00254 (Gradual) -> PASS
  [PitchShifter] Stereo Phase Lock Max Delta: 0 -> PASS

=== Running Parameter Tests ===
  [Parameters] Root keys: 12 | Scales: 9 | Modes: 4 -> PASS

========================================
  ALL TOYTUNE DSP TESTS PASSED (100%)
========================================
```

---

## 3. Analysis of Test Results

### 3.1 Fundamental Frequency Accuracy
- **110.0 Hz (A2):** Detected at 109.913 Hz (error 0.079%, confidence 99.47%).
- **220.0 Hz (A3):** Detected at 220.069 Hz (error 0.031%, confidence 93.40%).
- **440.0 Hz (A4):** Detected at 440.113 Hz (error 0.026%, confidence 96.38%).
- **880.0 Hz (A5):** Detected at 880.326 Hz (error 0.037%, confidence 98.07%).
- **Silence Gating:** Silence signal correctly tagged `isVoiced = false`, preventing false triggers.

### 3.2 Musical Targeting & Scale Masking
- **Chromatic Mode:** Accurately computed deviation of -35.0 cents for a sharp C#4 tone.
- **Major Scale Mode:** Correctly skipped out-of-scale C# in C Major and selected D4 as closest scale degree.
- **Custom Note Mask:** Selected exclusively active notes when custom bitmask was applied.

### 3.3 Correction Dynamics & Shifter
- **Hard Tune Snap:** Instant 0 ms pitch snap verified matching theoretical ratio $2^{40/1200} = 1.02337$.
- **Natural Mode:** Retune smoothing demonstrated gradual portamento curve.
- **Stereo Phase Coherence:** Max delta across Left and Right audio channels was zero ($0.0$), confirming phase-locked stereo imaging.
- **Plugin Latency Compensation (PDC):**
  - High Quality Mode: 771 samples (~17.4 ms at 44.1 kHz)
  - Low Latency Mode: 396 samples (~8.9 ms at 44.1 kHz)
