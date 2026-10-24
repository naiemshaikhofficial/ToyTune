# Learning Proposal & Technical Blueprint: Auto-Tune Pro 11 Harmony Player Engine for ToyTune

## 1. Classification & Scope
- **Classification**: New Major Architectural Subsystem (`ToyTune Harmony Player Engine`)
- **Scope**: 4-Voice Real-Time Vocal Harmonizer + Humanization Rack + Dynamics/EQ Strip + ToyTune UI View.

---

## 2. Deep-Dive Analysis: Auto-Tune Pro 11 "Harmony Player"

### A. What is Harmony Player?
In traditional vocal production, getting a 4-part lush vocal stack (e.g. Queen, Billie Eilish, Jacob Collier, Bollywood chorus) requires the singer to record 8 to 16 separate takes, manually singing 3rds, 5ths, and octaves, then editing, tuning, and aligning each take.

**Harmony Player** takes a **single monophonic lead vocal** track in real time and automatically generates up to **4 distinct, natural-sounding backing vocalists** that follow the song's musical key or live MIDI chord input.

---

### B. Core Functional Modules

```
                    ┌────────────────────────────────────────────────────────┐
                    │               INPUT VOCAL (Mono Lead)                  │
                    └──────────────────────────┬─────────────────────────────┘
                                               │
                       ┌───────────────────────┴───────────────────────┐
                       ▼                                               ▼
            [ Pitch Detection (F0) ]                        [ Lead Vocal Dry/Tuned ]
                       │                                               │
      ┌────────────────┼────────────────┬────────────────┐             │
      ▼                ▼                ▼                ▼             │
 [ Voice 1 ]      [ Voice 2 ]      [ Voice 3 ]      [ Voice 4 ]        │
  • Interval       • Interval       • Interval       • Interval        │
  • Formant        • Formant        • Formant        • Formant         │
  • Pan (L)        • Pan (CL)       • Pan (CR)       • Pan (R)         │
  • Level          • Level          • Level          • Level           │
      │                │                │                │             │
      └────────────────┼────────────────┴────────────────┘             │
                       ▼                                               │
       [ Humanization & Realism Engine ]                               │
        • Pitch Variation (Micro-detune, prevents comb-filtering)      │
        • Timing Variation (Loose humanized delay, 10–60 ms)           │
        • Transition Glide (Portamento legato between chords)          │
                       │                                               │
                       ▼                                               │
       [ Dynamics, EQ & Stereo Mastering Strip ]                       │
        • Noise Gate (Side-chained to lead vocal)                      │
        • EQ (High-pass / Low-pass cleanup)                            │
        • ADSR Envelope (Attack / Release swells)                      │
        • Stereo Width Spreader (M-S stereo expansion)                 │
                       │                                               │
                       └───────────────────────┬───────────────────────┘
                                               ▼
                                  [ MASTER STEREO MIX ]
```

---

### C. Detailed Breakdown of Each Parameter & Section

#### 1. The 4 Harmony Voice Channels
Each voice operates as an independent vocal character:
* **Pitch Interval Selector (`U`, `+3rd`, `-3rd`, `+5th`, `-4th`, `+7th`, `+Oct`, `-Oct`)**:
  * **Scale-Aware (Diatonic)**: Automatically calculates whether a 3rd should be Major (+4 semitones) or Minor (+3 semitones) based on the active scale (e.g., in C Major, singing C produces E, singing D produces F).
  * **Fixed**: Pure chromatic pitch shift.
  * **MIDI Mode**: Voice pitch follows notes held on a MIDI keyboard.
* **Formant Knob (Larynx Gender Shift)**:
  * Adjusts the apparent vocal tract length (0 to 200%).
  * A female singer can generate deep male baritone harmonies (formant < 100) or high angelic soprano harmonies (formant > 100) without chipmunk artifacts.
* **Pan Knob**:
  * Places each voice in the stereo field (e.g., Voice 1 hard left, Voice 2 hard right, Voice 3 mid-left, Voice 4 mid-right) for an ultra-wide studio wall of vocals.
* **Level Knob & VU Meter Ring**:
  * Individual volume balance (-inf to +6 dB) per backing singer.
* **Headphone (Solo) & Lock Icons**:
  * Allows auditioning each harmony voice in isolation and locking its settings when switching master presets.

---

#### 2. Humanization & Performance Controls
Why do synthesized harmonies often sound like a vocoder or cheap organ? Because identical pitch-shifted copies start at the exact same millisecond and have exact static pitch. Auto-Tune Pro solves this with:
* **Timing Variation (ms)**:
  * Introduces small pseudo-random millisecond offsets (10 to 60 ms) between the 4 voices. Voice 1 attacks at 0 ms, Voice 2 at 12 ms, Voice 3 at 24 ms. This perfectly replicates 4 separate human singers standing around a microphone.
* **Pitch Variation (cents)**:
  * Adds micro-cents natural drift (10 to 40 cents) so voices do not phase-cancel or sound sterile.
* **Transition Time (ms)**:
  * Controls portamento glide speed when chord degrees change, creating smooth legato choir transitions.
* **Naturalize (ms)**:
  * Blends micro-vibrato and throat breath dynamics from the lead vocal into the synthesized voices.

---

#### 3. Dynamics, EQ & Stereo Mastering Strip
* **Envelope (Attack & Release)**:
  * Allows creating ambient harmony pads (slow 300 ms attack) or tight pop backing vocal hits (0 ms attack, fast release).
* **Equalizer & Filter (6 / 12 / 18 / 24 dB/oct)**:
  * High-pass filter cuts muddy chest frequencies (<100 Hz), while low-pass cuts sibilance so harmonies sit comfortably behind the lead vocal without cluttering the mix.
* **Noise Gate (Side-Chain)**:
  * Detects when the lead singer stops singing, immediately muting the harmony channels to prevent room noise, headphone bleed, or breathing sounds from being multiplied 4 times.
* **Stereo Width (%)**:
  * Mid-Side spatial expander pushing backing harmonies to the far sides while leaving the phantom center open for the dry lead vocal.

---

## 3. ToyTune Architecture & Integration Plan

### A. Core DSP Architecture
1. **Multi-Voice Harmonizer Bank (`Source/DSP/HarmonyBank.h`)**:
   - 4 independent instances of ToyTune's high-precision 6-point Lagrange pitch shifters and formant filters.
   - Ring buffers with thread-safe sub-hop processing.
   - Real-time diatonic interval mapper hooked into `ScaleEngine`.
2. **Humanization Drift Generator (`Source/DSP/HarmonyHumanizer.h`)**:
   - Variable delay line per voice for loose timing variation.
   - Microtonal pitch jitter modulation.
3. **Harmony Mastering Strip (`Source/DSP/HarmonyChannelStrip.h`)**:
   - Integrated noise gate, 2-pole butterworth high-pass/low-pass EQ, and M-S stereo width expander.

### B. UI Design & Layout (Producer Toy Aesthetic)
1. **Top Mode Bar Navigation**:
   - Header bar receives a toggle: `[ Auto Mode ]  [ Harmony Player ]`.
2. **Harmony Player View (`Source/UI/HarmonyPlayerComponent.h`)**:
   - **Left**: Compact Pitch Circle + 4 primary knobs.
   - **Center**: 2x2 Grid of 4 Harmony Voice Cards with dark metallic chassis (`#0e0f12`), glowing white level rings, and custom interval selectors.
   - **Right-Center**: Humanization column (`Naturalize`, `Transition`, `Pitch Var`, `Timing Var`).
   - **Right**: Dynamics & Mastering rack (`Envelope`, `EQ`, `Gate`, `Stereo Width`).
   - **Bottom**: Retains the chromatic tracking keyboard.

---

## 4. Proposed Execution Phases
1. **Phase 1**: Implement `HarmonyBank.h` and diatonic scale-aware pitch calculations in `ScaleEngine`.
2. **Phase 2**: Add humanized micro-timing delay lines and pitch jitter engine.
3. **Phase 3**: Build `HarmonyChannelStrip.h` (Gate, EQ, Envelope, Stereo Width).
4. **Phase 4**: Register APVTS parameters for all 4 voices and mastering controls.
5. **Phase 5**: Create `HarmonyPlayerComponent` UI matching ToyTune's dark luxury monochrome design system.
6. **Phase 6**: Integrate mode switching in `HeaderBarComponent` & `PluginEditor`, verify builds, run test suite, compile installer, and push to GitHub.
