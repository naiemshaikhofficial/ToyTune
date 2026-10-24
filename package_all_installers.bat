@echo off
setlocal enabledelayedexpansion

echo ======================================================================
echo          PRODUCERTOY TOYTUNE - SECURE INSTALLER PACKAGER
echo ======================================================================
echo.

set "SCRIPT_DIR=%~dp0"
set "VST3_PE=%SCRIPT_DIR%build\ToyTune_artefacts\Release\VST3\ToyTune.vst3\Contents\x86_64-win\ToyTune.vst3"
set "STANDALONE_EXE=%SCRIPT_DIR%build\ToyTune_artefacts\Release\Standalone\ToyTune.exe"
set "OUTPUT_INSTALLER=%SCRIPT_DIR%Output\ProducerToy_ToyTune_Windows_Installer.exe"
set "ROOT_INSTALLER=%SCRIPT_DIR%ProducerToy_ToyTune_Installer.exe"
set "ISS_FILE=%SCRIPT_DIR%Installer\Windows\ToyTune_Windows_Installer.iss"

:: 1. Verify VST3 and Standalone Binaries Exist
echo [1/3] Verifying Release ToyTune binaries exist...
if not exist "%VST3_PE%" (
    echo [ERROR] Release VST3 binary not found: "%VST3_PE%"
    echo Compiling ToyTune VST3 first...
    cmake --build "%SCRIPT_DIR%build" --config Release --target ToyTune_VST3
    if not exist "%VST3_PE%" (
        echo [ERROR] VST3 build failed!
        pause
        exit /b 1
    )
)
echo [OK] ToyTune VST3 verified: "%VST3_PE%"

if not exist "%STANDALONE_EXE%" (
    echo [INFO] Standalone binary not found. Compiling Standalone...
    cmake --build "%SCRIPT_DIR%build" --config Release --target ToyTune_Standalone
)
if exist "%STANDALONE_EXE%" (
    echo [OK] ToyTune Standalone verified: "%STANDALONE_EXE%"
)
echo.

:: 2. Locate Inno Setup Compiler (ISCC.exe)
echo [2/3] Locating Inno Setup Compiler (ISCC.exe)...
if not defined ISCC_PATH (
    if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" (
        set "ISCC_PATH=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
    ) else if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
        set "ISCC_PATH=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    ) else if exist "C:\Program Files\Inno Setup 6\ISCC.exe" (
        set "ISCC_PATH=C:\Program Files\Inno Setup 6\ISCC.exe"
    ) else (
        for /f "delims=" %%I in ('where iscc.exe 2^>nul') do set "ISCC_PATH=%%I"
    )
)

if not exist "!ISCC_PATH!" (
    echo [ERROR] Inno Setup 6 ISCC.exe compiler not found!
    echo Please install Inno Setup 6 to compile the installer package.
    pause
    exit /b 1
)
echo [OK] Found Inno Setup Compiler: "!ISCC_PATH!"
echo.

:: 3. Compile Windows Installer
echo [3/3] Compiling Windows Installer Package...
if not exist "%SCRIPT_DIR%Output" mkdir "%SCRIPT_DIR%Output"

"!ISCC_PATH!" "%ISS_FILE%"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Inno Setup compilation failed!
    pause
    exit /b %ERRORLEVEL%
)

if exist "%OUTPUT_INSTALLER%" (
    copy /Y "%OUTPUT_INSTALLER%" "%ROOT_INSTALLER%" >nul
)

echo.
echo ======================================================================
echo                  TOYTUNE INSTALLER BUILD COMPLETE
echo ======================================================================
echo [PACKAGE]  Producer Toy ToyTune Windows 64-bit Installer
echo [OUTPUT]   %OUTPUT_INSTALLER%
echo [ROOT]     %ROOT_INSTALLER%
echo.
echo [INCLUDED COMPONENTS]
echo   - Universal VST3 64-bit Plugin (All DAWs)
echo   - Vendor VST3 Subfolder (Producer Toy\ToyTune.vst3)
echo   - Standalone Application 64-bit (Live Performance ^& Monitoring)
echo   - High-Res 3D Branding ^& Modern Inno Wizard Artwork
echo ======================================================================
echo.
