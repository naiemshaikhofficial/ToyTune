@echo off
setlocal

echo ========================================================
echo   Updating ToyTune VST3 in Common Files...
echo   Please ensure FL Studio or your DAW is closed before running!
echo ========================================================
echo.

set "SRC_DIR=%~dp0build\ToyTune_artefacts\Release\VST3\ToyTune.vst3"
set "DST_DIR1=C:\Program Files\Common Files\VST3\ToyTune.vst3"
set "DST_DIR2=C:\Program Files\Common Files\VST3\Producer Toy\ToyTune.vst3"

if not exist "%SRC_DIR%" (
    echo [ERROR] Release VST3 bundle not found at:
    echo "%SRC_DIR%"
    echo Please compile ToyTune first!
    echo.
    pause
    exit /b 1
)

:: Check for Administrative Privileges
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    if "%1"=="-elevated" (
        echo [ERROR] Administrative rights could not be obtained.
        echo Please right-click on this script and select 'Run as administrator'.
        pause
        exit /b 1
    )
    echo [INFO] Administrative permissions required to copy to Common Files\VST3.
    echo [INFO] Requesting administrator elevation...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd -ArgumentList '/c \"\"%~f0\"\" -elevated' -Verb RunAs"
    if %ERRORLEVEL% EQU 0 exit /b 0
    echo.
    echo [NOTICE] Elevation request was cancelled or unavailable.
    echo Please right-click on this file and choose 'Run as administrator'.
    pause
    exit /b 1
)

echo Copying VST3 bundle to "%DST_DIR1%"...
xcopy /E /Y /I /Q "%SRC_DIR%" "%DST_DIR1%"

echo Copying VST3 bundle to "%DST_DIR2%"...
xcopy /E /Y /I /Q "%SRC_DIR%" "%DST_DIR2%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] ToyTune VST3 installed successfully!
    echo Destinations:
    echo   - "%DST_DIR1%"
    echo   - "%DST_DIR2%"
    echo.
    echo You can now launch FL Studio and click 'Find installed plugins'.
) else (
    echo.
    echo [ERROR] Copy failed. If a file is locked, please close FL Studio and re-run.
)

if "%1"=="-elevated" (
    echo.
    echo Closing in 3 seconds...
    timeout /t 3 >nul
) else (
    echo.
    pause
)
