# XMage Launcher

A lightweight, cross-platform launcher for [XMage](https://github.com/magefree/mage) that makes it easy to get started without requiring Java to be pre-installed.

![Screenshot](/screenshots/launcher.jpg?raw=true)

## Download

| Platform | Download |
|----------|----------|
| Windows | [xmage-launcher-qt.zip](https://github.com/t-my/xmage-launcher-qt/releases/latest/download/windows.zip) |
| macOS | [XMage_Launcher.dmg](https://github.com/t-my/xmage-launcher-qt/releases/latest/download/macos.zip) |
| Linux | [XMage_Launcher.AppImage](https://github.com/t-my/xmage-launcher-qt/releases/latest/download/linux.zip) |

All releases are available on the [releases page](https://github.com/t-my/xmage-launcher-qt/releases).

## Features

- Downloads and manages XMage installations
- Auto-detects installed Java versions
- Downloads Java automatically if not found
- Support for multiple XMage installations
- Cross-platform (Windows, macOS, Linux)

## Quick Start

1. Download the launcher for your platform
2. Run the launcher
3. Click "Launch" - the launcher will download XMage and Java if needed

## Building from Source

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt install build-essential qt6-base-dev libzip-dev
```

**Linux (Arch):**
```bash
sudo pacman -S base-devel qt6-base libzip
```

**macOS:**
```bash
brew install qt6 libzip
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-qt6-base mingw-w64-x86_64-libzip make
```

### Build

```bash
mkdir build && cd build
qmake6 ..
make
```

## License

This project is open source. See the original [XMage project](https://github.com/magefree/mage) for more information.
