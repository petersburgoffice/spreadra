#!/bin/bash

# Create Universal Binary installer for Spreadra
# This installer will work on both Intel and Apple Silicon Macs

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/Builds_Universal"

# Get version
VERSION=$(cat "$PROJECT_ROOT/version.txt" 2>/dev/null || echo "1.0.0")

echo "🔧 Creating Universal Binary installer for Spreadra v$VERSION..."

# Check if Universal build exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "❌ Universal build not found. Please run ./build_universal.sh first"
    exit 1
fi

# Find built plugins
AU_PLUGIN=$(find "$BUILD_DIR" -name "*.component" -type d | head -1)
VST3_PLUGIN=$(find "$BUILD_DIR" -name "*.vst3" -type d | head -1)
AAX_PLUGIN=$(find "$BUILD_DIR" -name "*.aaxplugin" -type d | head -1)

echo "📦 Found plugins:"
[ -n "$AU_PLUGIN" ] && echo "   AU: $AU_PLUGIN"
[ -n "$VST3_PLUGIN" ] && echo "   VST3: $VST3_PLUGIN"
[ -n "$AAX_PLUGIN" ] && echo "   AAX: $AAX_PLUGIN"

# Verify architectures if plugins exist
echo ""
echo "🔍 Verifying Universal Binary architectures:"
if [ -n "$AU_PLUGIN" ]; then
    echo "AU Component:"
    lipo -info "$AU_PLUGIN/Contents/MacOS/"* 2>/dev/null | head -1 || echo "   AU binary not found"
fi

if [ -n "$VST3_PLUGIN" ]; then
    echo "VST3 Plugin:"
    lipo -info "$VST3_PLUGIN/Contents/MacOS/"* 2>/dev/null | head -1 || echo "   VST3 binary not found"
fi

if [ -n "$AAX_PLUGIN" ]; then
    echo "AAX Plugin:"
    lipo -info "$AAX_PLUGIN/Contents/MacOS/"* 2>/dev/null | head -1 || echo "   AAX binary not found"
fi

if false; then
fi

# Create temporary directory for installer files
TEMP_DIR=$(mktemp -d)
INSTALLER_ROOT="$TEMP_DIR/installer_root"

# Create directory structure
mkdir -p "$INSTALLER_ROOT/Library/Audio/Plug-Ins/Components"
mkdir -p "$INSTALLER_ROOT/Library/Audio/Plug-Ins/VST3"
mkdir -p "$INSTALLER_ROOT/Library/Application Support/Avid/Audio/Plug-Ins"
mkdir -p "$INSTALLER_ROOT/Applications"

# Copy plugins to installer root
echo ""
echo "📋 Copying plugins to installer package..."

if [ -n "$AU_PLUGIN" ]; then
    echo "   Copying AU plugin..."
    cp -R "$AU_PLUGIN" "$INSTALLER_ROOT/Library/Audio/Plug-Ins/Components/"
fi

if [ -n "$VST3_PLUGIN" ]; then
    echo "   Copying VST3 plugin..."
    cp -R "$VST3_PLUGIN" "$INSTALLER_ROOT/Library/Audio/Plug-Ins/VST3/"
fi

if [ -n "$AAX_PLUGIN" ]; then
    echo "   Copying AAX plugin..."
    cp -R "$AAX_PLUGIN" "$INSTALLER_ROOT/Library/Application Support/Avid/Audio/Plug-Ins/"
fi

if false; then
fi

# Create installer package
INSTALLER_NAME="Spreadra_${VERSION}_Universal_macOS_Installer.pkg"
INSTALLER_PATH="$SCRIPT_DIR/$INSTALLER_NAME"

echo "🔨 Building installer package..."
pkgbuild --root "$INSTALLER_ROOT" \
         --identifier "com.spreadra.universal" \
         --version "$VERSION" \
         --install-location "/" \
         "$INSTALLER_PATH"

# Clean up
rm -rf "$TEMP_DIR"

if [ -f "$INSTALLER_PATH" ]; then
    INSTALLER_SIZE=$(du -h "$INSTALLER_PATH" | cut -f1)
    echo ""
    echo "✅ Universal Binary installer created successfully!"
    echo "📦 File: $INSTALLER_PATH"
    echo "📏 Size: $INSTALLER_SIZE"
    echo ""
    echo "🎯 This installer works on both Intel and Apple Silicon Macs"
    echo "🚀 Ready for distribution!"
    echo ""
    echo "📋 Installation locations:"
    [ -n "$AU_PLUGIN" ] && echo "   • AU: ~/Library/Audio/Plug-Ins/Components/"
    [ -n "$VST3_PLUGIN" ] && echo "   • VST3: ~/Library/Audio/Plug-Ins/VST3/"
    [ -n "$AAX_PLUGIN" ] && echo "   • AAX: ~/Library/Application Support/Avid/Audio/Plug-Ins/"
else
    echo "❌ Failed to create installer"
    exit 1
fi 