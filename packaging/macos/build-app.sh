#!/bin/bash
#
# Build script for creating XMage Launcher macOS app bundle
#
# Prerequisites:
#   - Qt6 (installed via Homebrew: brew install qt6)
#   - libzip (installed via Homebrew: brew install libzip)
#   - Xcode command line tools
#
# Usage:
#   ./packaging/macos/build-app.sh
#
# Output:
#   XMage_Launcher.app in the project root
#   XMage_Launcher.dmg (optional disk image)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APP_NAME="xmage-launcher-qt"
APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"

echo "=== XMage Launcher macOS Builder ==="
echo "Project root: $PROJECT_ROOT"
echo ""

# Step 1: Build the application
echo "=== Step 1: Building application ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Find qmake (prefer Qt6 from Homebrew)
QMAKE=$(which qmake6 2>/dev/null || which qmake 2>/dev/null)
if [ -z "$QMAKE" ]; then
    # Try Homebrew paths
    if [ -f "/opt/homebrew/opt/qt6/bin/qmake" ]; then
        QMAKE="/opt/homebrew/opt/qt6/bin/qmake"
    elif [ -f "/usr/local/opt/qt6/bin/qmake" ]; then
        QMAKE="/usr/local/opt/qt6/bin/qmake"
    else
        echo "Error: qmake not found. Install Qt6: brew install qt6"
        exit 1
    fi
fi

echo "Using qmake: $QMAKE"
"$QMAKE" "$PROJECT_ROOT"
make -j$(sysctl -n hw.ncpu)
echo "Build complete: $APP_BUNDLE"
echo ""

# Step 2: Deploy Qt frameworks
echo "=== Step 2: Deploying Qt frameworks ==="

# Find macdeployqt
MACDEPLOYQT=$(dirname "$QMAKE")/macdeployqt
if [ ! -f "$MACDEPLOYQT" ]; then
    MACDEPLOYQT=$(which macdeployqt 2>/dev/null)
fi

if [ -z "$MACDEPLOYQT" ] || [ ! -f "$MACDEPLOYQT" ]; then
    echo "Error: macdeployqt not found"
    exit 1
fi

echo "Using macdeployqt: $MACDEPLOYQT"
"$MACDEPLOYQT" "$APP_BUNDLE"
echo "Qt frameworks deployed"
echo ""

# Step 3: Code sign the application
echo "=== Step 3: Code signing ==="
codesign --force --deep --sign - "$APP_BUNDLE"
echo "App bundle signed (ad-hoc)"
echo ""

# Step 4: Copy to project root
echo "=== Step 4: Copying app bundle ==="
OUTPUT_APP="$PROJECT_ROOT/XMage_Launcher.app"
rm -rf "$OUTPUT_APP"
cp -R "$APP_BUNDLE" "$OUTPUT_APP"
echo "Output: $OUTPUT_APP"
echo ""

# Step 5: Optionally create DMG
if [ "$1" = "--dmg" ]; then
    echo "=== Step 5: Creating DMG ==="
    DMG_NAME="XMage_Launcher.dmg"
    DMG_PATH="$PROJECT_ROOT/$DMG_NAME"

    # Remove old DMG if exists
    rm -f "$DMG_PATH"

    # Create a temporary directory for DMG contents
    DMG_TEMP="$BUILD_DIR/dmg_temp"
    rm -rf "$DMG_TEMP"
    mkdir -p "$DMG_TEMP"
    cp -R "$OUTPUT_APP" "$DMG_TEMP/"
    ln -s /Applications "$DMG_TEMP/Applications"

    # Create DMG
    hdiutil create -volname "XMage Launcher" -srcfolder "$DMG_TEMP" -ov -format UDZO "$DMG_PATH"

    rm -rf "$DMG_TEMP"
    echo "Output: $DMG_PATH"
    ls -lh "$DMG_PATH"
fi

echo ""
echo "=== Build complete ==="
ls -lh "$OUTPUT_APP"
