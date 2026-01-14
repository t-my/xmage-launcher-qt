#!/bin/bash
#
# Build script for creating XMage Launcher AppImage
#
# Prerequisites:
#   - Qt6 development packages (qt6-base-dev or similar)
#   - libzip development package
#   - curl (for downloading appimagetool)
#
# Usage:
#   ./packaging/linux/build-appimage.sh
#
# Output:
#   linux.zip containing the AppImage and empty java/ and xmage/ folders

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APPDIR="$BUILD_DIR/AppDir"

echo "=== XMage Launcher AppImage Builder ==="
echo "Project root: $PROJECT_ROOT"
echo ""

# Step 1: Build the application
echo "=== Step 1: Building application ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
qmake6 "$PROJECT_ROOT" || qmake "$PROJECT_ROOT"
make -j$(nproc)
strip xmage-launcher-qt
echo "Build complete: $BUILD_DIR/xmage-launcher-qt"
echo ""

# Step 2: Create AppDir structure
echo "=== Step 2: Creating AppDir structure ==="
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/plugins/platforms"
mkdir -p "$APPDIR/usr/plugins/tls"
mkdir -p "$APPDIR/usr/plugins/xcbglintegrations"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

# Copy binary
cp "$BUILD_DIR/xmage-launcher-qt" "$APPDIR/usr/bin/"

# Copy desktop file and icon
cp "$SCRIPT_DIR/xmage-launcher-qt.desktop" "$APPDIR/usr/share/applications/"
cp "$PROJECT_ROOT/resources/icon-mage.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/xmage-launcher-qt.png"

echo "AppDir structure created"
echo ""

# Step 3: Copy Qt libraries
# Note: We bundle Qt libs for consistency, but rely on system ICU to keep size down
echo "=== Step 3: Copying Qt libraries ==="
cp /usr/lib/libQt6Core.so.6 "$APPDIR/usr/lib/"
cp /usr/lib/libQt6Gui.so.6 "$APPDIR/usr/lib/"
cp /usr/lib/libQt6Widgets.so.6 "$APPDIR/usr/lib/"
cp /usr/lib/libQt6Network.so.6 "$APPDIR/usr/lib/"
cp /usr/lib/libQt6DBus.so.6 "$APPDIR/usr/lib/"

# Copy other required libraries
cp /usr/lib/libzip.so.5 "$APPDIR/usr/lib/"
cp /usr/lib/libdouble-conversion.so.3 "$APPDIR/usr/lib/"
cp /usr/lib/libb2.so.1 "$APPDIR/usr/lib/"
cp /usr/lib/libpcre2-16.so.0 "$APPDIR/usr/lib/"
cp /usr/lib/libmd4c.so.0 "$APPDIR/usr/lib/"

# Copy SSL libraries (required for HTTPS/TLS)
cp /usr/lib/libssl.so.3 "$APPDIR/usr/lib/"
cp /usr/lib/libcrypto.so.3 "$APPDIR/usr/lib/"

echo "Libraries copied"
echo ""

# Step 4: Copy Qt plugins
echo "=== Step 4: Copying Qt plugins ==="

# Platform plugins (for X11/Wayland display)
cp /usr/lib/qt6/plugins/platforms/libqxcb.so "$APPDIR/usr/plugins/platforms/" 2>/dev/null || true
cp /usr/lib/qt6/plugins/platforms/libqwayland*.so "$APPDIR/usr/plugins/platforms/" 2>/dev/null || true

# XCB GL integrations
cp /usr/lib/qt6/plugins/xcbglintegrations/*.so "$APPDIR/usr/plugins/xcbglintegrations/" 2>/dev/null || true

# TLS plugins (required for HTTPS)
cp /usr/lib/qt6/plugins/tls/libqopensslbackend.so "$APPDIR/usr/plugins/tls/"
cp /usr/lib/qt6/plugins/tls/libqcertonlybackend.so "$APPDIR/usr/plugins/tls/"

echo "Plugins copied"
echo ""

# Step 5: Create AppRun script
echo "=== Step 5: Creating AppRun script ==="
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms"
exec "${HERE}/usr/bin/xmage-launcher-qt" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Create required symlinks in AppDir root
ln -sf usr/share/applications/xmage-launcher-qt.desktop "$APPDIR/xmage-launcher-qt.desktop"
ln -sf usr/share/icons/hicolor/256x256/apps/xmage-launcher-qt.png "$APPDIR/xmage-launcher-qt.png"
ln -sf usr/share/icons/hicolor/256x256/apps/xmage-launcher-qt.png "$APPDIR/.DirIcon"

echo "AppRun and symlinks created"
echo ""

# Step 6: Download appimagetool if needed
echo "=== Step 6: Preparing appimagetool ==="
APPIMAGETOOL="$BUILD_DIR/appimagetool-x86_64.AppImage"
if [ ! -f "$APPIMAGETOOL" ]; then
    echo "Downloading appimagetool..."
    curl -L -o "$APPIMAGETOOL" https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "$APPIMAGETOOL"
fi
echo "appimagetool ready"
echo ""

# Step 7: Create AppImage
echo "=== Step 7: Creating AppImage ==="
APPIMAGE_OUTPUT="$BUILD_DIR/XMage_Launcher-x86_64.AppImage"
"$APPIMAGETOOL" "$APPDIR" "$APPIMAGE_OUTPUT"
echo "AppImage created"
echo ""

# Step 8: Create distribution folder with AppImage and empty folders
echo "=== Step 8: Creating distribution ==="
DIST_DIR="$BUILD_DIR/dist"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
cp "$APPIMAGE_OUTPUT" "$DIST_DIR/"
mkdir -p "$DIST_DIR/java"
mkdir -p "$DIST_DIR/xmage"
echo "Distribution folder created with empty java/ and xmage/ folders"
echo ""

# Step 9: Create zip file
echo "=== Step 9: Creating linux.zip ==="
cd "$DIST_DIR"
zip -r -q "$PROJECT_ROOT/linux.zip" .
echo ""
echo "=== Build complete ==="
echo "Output: $PROJECT_ROOT/linux.zip"
ls -lh "$PROJECT_ROOT/linux.zip"
