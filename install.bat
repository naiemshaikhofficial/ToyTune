@echo off
echo ========================================================
echo   ToyTune by Producer Toy - VST3 Installer
echo ========================================================
echo.

set SOURCE_VST3=%~dp0build\ToyTune_artefacts\Release\VST3\ToyTune.vst3
set TARGET_DIR=C:\Program Files\Common Files\VST3\Producer Toy
set USER_VST3=%LOCALAPPDATA%\Programs\Common\VST3\Producer Toy

if not exist "%SOURCE_VST3%" (
    echo [ERROR] ToyTune.vst3 build not found!
    echo Please compile the plugin first using CMake or build_release.bat
    pause
    exit /b 1
)

echo [1/2] Installing to User VST3 directory...
if not exist "%USER_VST3%" mkdir "%USER_VST3%"
xcopy /E /I /Y /Q "%SOURCE_VST3%" "%USER_VST3%\ToyTune.vst3"
echo       Installed in: %USER_VST3%\ToyTune.vst3

echo.
echo [2/2] Attempting install to System VST3 (Program Files)...
net session >nul 2>&1
if %errorlevel% == 0 (
    if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
    xcopy /E /I /Y /Q "%SOURCE_VST3%" "%TARGET_DIR%\ToyTune.vst3"
    echo       Installed in: %TARGET_DIR%\ToyTune.vst3
) else (
    echo       [NOTE] To install to System Program Files directory as well,
    echo              Right-click this 'install.bat' and select 'Run as administrator'.
)

echo.
echo ========================================================
echo   ToyTune VST3 installation complete!
echo   Open your DAW (FL Studio, Ableton, Cubase, etc.)
echo   and perform a Plugin Rescan.
echo ========================================================
pause
