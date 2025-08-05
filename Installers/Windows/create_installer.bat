@echo off
echo.
echo 🎵 Creating Spreadra Windows Installer...
echo.

REM Check if NSIS is installed
where makensis >nul 2>nul
if %errorlevel% neq 0 (
    echo ❌ Error: NSIS (Nullsoft Scriptable Install System) is not installed or not in PATH
    echo.
    echo 💡 Please install NSIS from: https://nsis.sourceforge.io/Download
    echo    Make sure to add NSIS to your system PATH during installation
    echo.
    pause
    exit /b 1
)

REM Check if VST3 plugin exists
if not exist "..\..\build\Spreadra_artefacts\VST3\Spreadra.vst3" (
    echo ❌ Error: VST3 plugin not found
    echo 💡 Please build the plugin first using the build scripts
    echo    Expected location: ..\..\build\Spreadra_artefacts\VST3\Spreadra.vst3
    echo.
    pause
    exit /b 1
)

REM Get version from version.txt
set /p VERSION=<..\..\version.txt
echo 📦 Version: %VERSION%
echo.

REM Update version in NSIS script
powershell -Command "(gc spreadra_installer.nsi) -replace '!define PRODUCT_VERSION \".*\"', '!define PRODUCT_VERSION \"%VERSION%\"' | Out-File -encoding ASCII spreadra_installer_temp.nsi"

echo 🔨 Building Windows installer...
makensis spreadra_installer_temp.nsi

if %errorlevel% equ 0 (
    echo.
    echo ✅ Windows Installer created successfully!
    echo 📦 File: Spreadra_%VERSION%_Windows_Installer.exe
    
    REM Get file size
    for %%A in ("Spreadra_%VERSION%_Windows_Installer.exe") do (
        set size=%%~zA
        set /a sizeMB=!size!/1024/1024
    )
    echo 📏 Size: !sizeMB! MB
    echo.
    echo 💡 Test the installer before distribution!
    echo.
    echo 📋 Installer includes:
    echo    • VST3 plugin installation
    echo    • Desktop shortcut (optional)
    echo    • Start Menu shortcuts (optional)
    echo    • Uninstaller creation
    echo    • Registry entries
) else (
    echo.
    echo ❌ Error: Failed to create installer
)

REM Clean up temporary file
del spreadra_installer_temp.nsi 2>nul

echo.
pause 