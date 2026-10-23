# ToyTune — Build & Installation Guide
**Developer:** Producer Toy  
**Plugin:** ToyTune Real-Time Vocal Pitch Correction  

---

## 1. Prerequisites
- **Operating System:** Windows 10 or 11 (64-bit)
- **Compiler:** Microsoft Visual C++ (MSVC) from Visual Studio 2022 (Build Tools or Community/Professional)
- **Build Generator:** CMake 3.22+
- **JUCE Framework:** JUCE 8 (automatically detected from local directory or fetched via FetchContent)

---

## 2. Configuration & Compilation

### Step 1: Open PowerShell or Developer Command Prompt
Navigate to the ToyTune project directory:
```powershell
cd d:\Website\ProducerToy-Plugins\ToyTune
```

### Step 2: Configure CMake
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
```

### Step 3: Build Plugin & Test Targets
```powershell
cmake --build build --config Release
```

The build produces:
- **VST3 Plugin:** `build/ToyTune_artefacts/Release/VST3/ToyTune.vst3`
- **Standalone Executable:** `build/ToyTune_artefacts/Release/Standalone/ToyTune.exe`
- **DSP Unit Test Suite:** `build/Release/ToyTuneTests.exe`

---

## 3. Running Automated Tests
```powershell
./build/Release/ToyTuneTests.exe
```

---

## 4. DAW Installation

### Windows VST3 Standard Directory
Copy `ToyTune.vst3` to the standard system VST3 location:
```powershell
Copy-Item -Path "build/ToyTune_artefacts/Release/VST3/ToyTune.vst3" -Destination "$env:CommonProgramFiles\VST3" -Recurse -Force
```

### Compatible DAWs
- FL Studio 20 / 21 / 2024
- Ableton Live 11 / 12
- Cubase 12 / 13 / 14 / Nuendo
- Studio One 6 / 7
- Reaper 6 / 7
- Bitwig Studio
