; ==============================================================
; Producer Toy ToyTune Official Windows Audio Plugin Installer
; Supported Formats: Universal VST3 (64-bit), Standalone Application (64-bit)
; ==============================================================

[Setup]
AppName=Producer Toy ToyTune
AppVersion=1.0.0
AppPublisher=Producer Toy
AppPublisherURL=https://producertoy.com
AppSupportURL=https://producertoy.com
DefaultDirName={commoncf64}\VST3\ToyTune.vst3
DisableDirPage=yes
DefaultGroupName=Producer Toy
DisableProgramGroupPage=yes
OutputBaseFilename=ProducerToy_ToyTune_Windows_Installer
OutputDir=..\..\Output
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UsedUserAreasWarning=no
WizardStyle=modern
WizardImageFile=..\wizard_large.bmp,..\wizard_large_2x.bmp
WizardSmallImageFile=..\wizard_small.bmp
SetupIconFile=..\toytune.ico
UninstallFilesDir={commoncf64}\ProducerToy\ToyTune
DisableReadyPage=no

[Types]
Name: "full"; Description: "Standard Installation (VST3 & Standalone - Recommended)"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 (64-bit) — Universal DAW Plugin (FL Studio, Ableton Live, Cubase, Studio One, Reaper, Bitwig)"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Standalone Application (64-bit) — Live Vocal Tuning & Real-Time Mic Monitoring"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create a Desktop shortcut for Standalone App"; GroupDescription: "Additional Shortcuts:"; Components: standalone

[Messages]
FinishedHeadingLabel=Producer Toy ToyTune Installed
FinishedLabel=Producer Toy ToyTune has been successfully installed on your computer.%n%nUniversal compatibility ready for all DAWs:%n• FL Studio: Effects -> Pitch / Modulation -> ToyTune%n• Ableton Live: Plug-ins -> VST3 -> Producer Toy%n• Cubase / Nuendo: Pitch Shift -> ToyTune%n• Studio One: Effects -> Producer Toy -> ToyTune%n• Reaper: VST3: ToyTune (Producer Toy)%n• Bitwig Studio: Devices -> Audio FX -> ToyTune%n%nStandalone mode is ready for zero-latency live vocal performance.%nClick Finish to complete setup.

[Files]
; 1a. Official Universal VST3 Plugin Bundle for DAWs (Root VST3 folder)
Source: "..\..\build\ToyTune_artefacts\Release\VST3\ToyTune.vst3\*"; DestDir: "{commoncf64}\VST3\ToyTune.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3; Excludes: "*.pdb,*.ilk,*.exp,*.lib,*.obj,*.cpp,*.h,*.git*,*.md,*.txt"

; 1b. Producer Toy Vendor VST3 Subfolder (For DAWs organizing by vendor: FL Studio, Cubase, Studio One)
Source: "..\..\build\ToyTune_artefacts\Release\VST3\ToyTune.vst3\*"; DestDir: "{commoncf64}\VST3\Producer Toy\ToyTune.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3; Excludes: "*.pdb,*.ilk,*.exp,*.lib,*.obj,*.cpp,*.h,*.git*,*.md,*.txt"

; 2. Standalone Application Executable & Icon
Source: "..\..\build\ToyTune_artefacts\Release\Standalone\ToyTune.exe"; DestDir: "{autopf}\Producer Toy\ToyTune"; Flags: ignoreversion; Components: standalone; Excludes: "*.pdb,*.ilk,*.exp,*.lib,*.obj"
Source: "..\toytune.ico"; DestDir: "{autopf}\Producer Toy\ToyTune"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\Producer Toy\ToyTune"; Filename: "{autopf}\Producer Toy\ToyTune\ToyTune.exe"; IconFilename: "{autopf}\Producer Toy\ToyTune\toytune.ico"; Components: standalone
Name: "{autodesktop}\ToyTune"; Filename: "{autopf}\Producer Toy\ToyTune\ToyTune.exe"; IconFilename: "{autopf}\Producer Toy\ToyTune\toytune.ico"; Components: standalone; Tasks: desktopicon

[Dirs]
Name: "{userappdata}\ProducerToy\Licenses"; Flags: uninsneveruninstall
Name: "{userdocs}\ProducerToy\ToyTune\Presets"; Flags: uninsneveruninstall

[Run]
Filename: "{autopf}\Producer Toy\ToyTune\ToyTune.exe"; Description: "Launch ToyTune Standalone now"; Flags: postinstall nowait skipifsilent unchecked; Components: standalone
