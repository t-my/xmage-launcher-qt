# xmage-launcher-qt
An alternate launcher for [the XMage project](https://github.com/magefree/mage) written in C++ using the Qt GUI framework.

The goal is to make it easier for users to get started without requiring Java to be installed as well as providing additional features such as support for multiple installations and auto-detection of installed Java versions.

![Screenshot](/screenshots/launcher.jpg?raw=true)

## For Users
**Windows:** Statically linked binaries for Windows will be available on the [releases page](https://github.com/weirddan455/xmage-launcher-qt/releases). Simply download and run the .exe file. No external dependencies are required.

**Linux:** Download the AppImage from the [releases page](https://github.com/weirddan455/xmage-launcher-qt/releases), make it executable (`chmod +x`), and run it.

**macOS:** Download the DMG from the [releases page](https://github.com/weirddan455/xmage-launcher-qt/releases), open it, and drag the app to Applications.

## For Developers
### Linux
First, you will need to install some prerequisites:

Arch Linux:
```
sudo pacman -S base-devel git qt6-base libzip curl --needed
```

Ubuntu:
```
sudo apt install build-essential git qt6-base-dev libzip-dev curl
```

Qt Creator can optionally be installed and used as an IDE. It's in both Arch and Ubuntu repos as "qtcreator".

#### Building from the command line

```
git clone https://github.com/weirddan455/xmage-launcher-qt.git
cd xmage-launcher-qt
mkdir build && cd build
qmake6 ..
make
```

#### Creating an AppImage for distribution

```
./packaging/linux/build-appimage.sh
```

This creates `XMage_Launcher-x86_64.AppImage` in the project root. The script handles building, bundling Qt libraries, and packaging automatically.

### macOS
Install prerequisites using Homebrew:
```
brew install qt6 libzip
```

#### Building from the command line

```
git clone https://github.com/weirddan455/xmage-launcher-qt.git
cd xmage-launcher-qt
mkdir build && cd build
qmake6 ..
make
```

The build automatically runs `macdeployqt` to bundle Qt frameworks and `codesign` to sign the app. The resulting `xmage-launcher-qt.app` can be run directly or moved to `/Applications`.

#### Creating a DMG for distribution

```
./packaging/macos/build-app.sh --dmg
```

This creates `XMage_Launcher.dmg` in the project root.

### Windows
The easiest way I've found to set up a build enviornment in Windows is to use MSYS2.  Download and run the installer from their website: https://www.msys2.org/

Next, open the MSYS2 terminal and run "pacman -Syu" to update (may need to be run multiple times to get all updates.)

Once updated, install the prerequisites:
```
pacman -S base-devel mingw-w64-i686-toolchain mingw-w64-i686-qt5 mingw-w64-i686-libzip --needed
```

Then you may build from the command line.  First create and navigate to a build directory.  The C drive in MSYS2 is found in /c if you don't want to build inside the MSYS2 home directory.

The following commands should be entered in the MSYS2 MinGW 32 bit terminal (this sets needed environment variables up that aren't in the normal MSYS2 terminal.)

```
cd /path/to/build
qmake /path/to/xmage-launcher-qt
make
```

NOTE:  The above will make a dynamically linked binary that will only be usable inside the MSYS2 environment.  If you would like to build a static binary instead, install the mingw-w64-i686-qt5-static package (mingw-w64-i686-qt5 is not needed in this case) You must then use qmake from the static build (it is not in a default path.)  Example:

```
cd /path/to/build
/mingw32/qt5-static/bin/qmake.exe /path/to/xmage-launcher-qt
make
```

For debugging purposes, you may prefer a dyanmic build as the static debug build is very large.

To setup the Qt Creator IDE, you can either install it with MSYS2 (package name mingw-w64-i686-qt-creator) or download it from the Qt website (though note the latter requires creating an account with Qt.)

You will then need to create a kit pointing to the MSYS2 build environment if it is not auto-detected.  The offical Qt installer will provide its own build environment but it does not have a package manager so you will run into issues linking with third pary libraries (currently just libzip.)

To do so, open the xmage-launcher-qt project, click Projects on the left panel, then "Manage Kits."  Add a new Kit.  Go to Qt Versions tab and add.  Qmake location is "C:\msys64\mingw32\bin\qmake.exe" or "C:\msys64\mingw32\qt-static\bin\qmake.exe" for the static build.  Go to the Compilers tab and add a C and C++ compiler.  C compiler is located at "C:\msys64\mingw32\bin\gcc.exe" and C++ is "C:\msys64\mingw32\bin\g++.exe"  Go to Debugger tab and add do the same thing.  It is located in "C:\msys64\mingw32\bin\gdb.exe"
