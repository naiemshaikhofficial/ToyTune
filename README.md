# ToyTune by Producer Toy

**ToyTune** is a real-time vocal pitch correction and tuning audio plugin developed by **Producer Toy**.

## Overview
- **Platform:** Windows x64 (macOS compatible)
- **Plugin Formats:** VST3, Standalone Application
- **Framework:** JUCE 8 (C++17) & CMake
- **Audio Processing:** Hard real-time safe, lock-free telemetry, monophonic F0 tracking with CMNDF/YIN, dual-head crossfaded pitch shifting, musical scale targeting, formant envelope preservation.
- **UI:** Custom native JUCE vector interface with Producer Toy luxury charcoal/black styling, scrolling pitch lane trace, interactive 12-key scale keyboard, and real-time meters.

## Build Requirements
- CMake 3.22 or higher
- Visual Studio 2022 (MSVC v143 or newer)
- Windows 10/11 x64

## Build Instructions
```powershell
# Configure CMake
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build Release targets
cmake --build build --config Release
```

## Running Automated DSP Tests
```powershell
./build/Release/ToyTuneTests.exe
```
