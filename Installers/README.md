# Reverbix Installers

This directory contains scripts and configurations for creating professional installers for Reverbix on both macOS and Windows platforms.

## Quick Start

To create installers for both platforms, run from the project root:

```bash
./create_installers.sh
```

This will automatically:
1. Build Universal Binary plugins for macOS (if not already built)
2. Create a macOS installer package (.pkg)
3. Show instructions for creating Windows installer

## Directory Structure

```
Installers/
├── macOS/
│   └── create_universal_installer.sh    # macOS installer creation script
├── Windows/
│   ├── create_installer.bat             # Windows installer creation script
│   ├── reverbix_installer.nsi           # NSIS installer script
│   └── license.txt                      # License agreement
└── README.md                            # This file
```

## macOS Installer

### Prerequisites
- Xcode Command Line Tools
- CMake 3.15+
- Universal Binary plugins built (automatic)

### Usage
```bash
# From project root
./create_installers.sh
# OR directly
./Installers/macOS/create_universal_installer.sh
```

### Features
- **Universal Binary**: Works on both Intel and Apple Silicon Macs
- **Multiple Formats**: Installs AU, VST3, AAX (if available), and Standalone
- **System Integration**: Uses native macOS installer (.pkg)
- **Version Management**: Automatically reads version from version.txt

### Installation Locations
- AU: `~/Library/Audio/Plug-Ins/Components/`
- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- AAX: `~/Library/Application Support/Avid/Audio/Plug-Ins/`
- Standalone: `/Applications/`

## Windows Installer

### Prerequisites
- Windows machine
- NSIS (Nullsoft Scriptable Install System) - [Download](https://nsis.sourceforge.io/Download)
- Visual Studio Build Tools or Visual Studio
- CMake 3.15+

### Setup
1. Install NSIS and add to system PATH
2. Build plugins on Windows:
   ```cmd
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```

### Usage
```cmd
cd Installers\Windows
create_installer.bat
```

### Features
- **Component Selection**: User can choose which components to install
- **Multiple Formats**: VST3 (required), AAX, Standalone (optional)
- **Desktop/Start Menu Shortcuts**: Optional shortcuts creation
- **Uninstaller**: Complete removal functionality
- **Registry Integration**: Proper Windows registry entries
- **Version Detection**: Handles upgrades automatically

### Installation Locations
- VST3: `C:\Program Files\Common Files\VST3\`
- AAX: `C:\Program Files\Common Files\Avid\Audio\Plug-Ins\`
- Standalone: `C:\Program Files\Reverbix\`

## Build Process

### 1. Universal Binary Build (macOS)
The `build_universal.sh` script creates Universal Binary plugins compatible with both Intel and Apple Silicon:

```bash
./build_universal.sh
```

This configures CMake with:
- `CMAKE_OSX_ARCHITECTURES="x86_64;arm64"`
- `CMAKE_OSX_DEPLOYMENT_TARGET=10.13`

### 2. macOS Installer Creation
The macOS installer script:
1. Verifies Universal Binary builds exist
2. Creates temporary installer structure
3. Copies plugins to appropriate directories
4. Builds signed .pkg installer using `pkgbuild`

### 3. Windows Installer Creation
The Windows installer process:
1. Verifies NSIS installation
2. Checks for built plugin files
3. Updates version in NSIS script
4. Compiles installer executable

## Customization

### macOS Installer
- Edit `Installers/macOS/create_universal_installer.sh`
- Modify installer identifier: `com.reverbix.universal`
- Change installation paths if needed

### Windows Installer
- Edit `Installers/Windows/reverbix_installer.nsi`
- Modify `PRODUCT_*` defines for branding
- Add/remove installation components
- Customize installer pages and dialogs

## Testing

### macOS
```bash
# Install
sudo installer -pkg "Installers/macOS/Reverbix_X.X.X_Universal_macOS_Installer.pkg" -target /

# Verify installation
ls ~/Library/Audio/Plug-Ins/Components/Reverbix*
ls ~/Library/Audio/Plug-Ins/VST3/Reverbix*
```

### Windows
1. Run the installer executable as Administrator
2. Verify installation in respective directories
3. Test uninstaller functionality

## Troubleshooting

### macOS Issues
- **"No Universal Binary found"**: Run `./build_universal.sh` first
- **Permission denied**: Ensure scripts are executable (`chmod +x`)
- **pkgbuild not found**: Install Xcode Command Line Tools

### Windows Issues
- **"makensis not found"**: Install NSIS and add to PATH
- **"Plugin not found"**: Build plugins first using CMake
- **Permission denied**: Run as Administrator

## Distribution

### macOS
- The .pkg installer can be distributed directly
- Consider code signing for distribution outside App Store
- Test on both Intel and Apple Silicon machines

### Windows
- The .exe installer can be distributed directly
- Consider code signing for trust and security
- Test on different Windows versions (10, 11)

## Version Management

Both installers automatically read version from `version.txt` in the project root. Update this file to change installer versions:

```bash
echo "1.0.1" > version.txt
```

## Security Notes

- macOS installers require admin privileges for system directory installation
- Windows installers require admin privileges and may trigger UAC prompts
- Both installers include proper uninstall functionality
- Registry entries on Windows follow Microsoft guidelines

---

**For support or questions about the installer system, please refer to the project documentation or create an issue.** 