QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# Executable metadata. On Windows these populate the PE VERSIONINFO resource;
# an unsigned binary with no version/company info looks more suspicious to AV
# heuristics, so we always embed proper provenance.
VERSION = 0.2.0
QMAKE_TARGET_COMPANY     = "XMage"
QMAKE_TARGET_PRODUCT     = "XMage Launcher"
QMAKE_TARGET_DESCRIPTION = "Launcher for XMage (Magic: The Gathering)"
QMAKE_TARGET_COPYRIGHT   = "GPL-3.0 — github.com/t-my/xmage-launcher-qt"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/downloadmanager.cpp \
    src/zipextractthread.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/settings.cpp \
    src/settingsdialog.cpp \
    src/unzipthread.cpp \
    src/xmageprocess.cpp

HEADERS += \
    src/downloadmanager.h \
    src/zipextractthread.h \
    src/mainwindow.h \
    src/settings.h \
    src/settingsdialog.h \
    src/unzipthread.h \
    src/xmageprocess.h

FORMS += \
    forms/mainwindow.ui \
    forms/settingsdialog.ui

# Deployment rules disabled — app is distributed as a zip/dmg, not installed to /opt

RESOURCES += \
    resources/resources.qrc

macx {
    INCLUDEPATH += /opt/homebrew/opt/libzip/include
    LIBS += -L/opt/homebrew/opt/libzip/lib -lzip
    ICON = resources/icon-mage.icns

    # Copy settings.json inside .app bundle, deploy Qt frameworks, and sign
    QMAKE_POST_LINK += cp $$PWD/settings.json $${TARGET}.app/Contents/MacOS/ && \
        macdeployqt $${TARGET}.app && \
        codesign --force --deep --sign - $${TARGET}.app

    # Clean up .app bundle on make clean
    QMAKE_CLEAN += -r $${TARGET}.app
}
linux {
    LIBS += -lzip
    QMAKE_POST_LINK += cp $$PWD/settings.json .
    QMAKE_CLEAN += settings.json
}
win32 {
    RC_ICONS = resources/icon-mage.ico
    LIBS += -lzip -lbz2 -llzma -lmsi
    QMAKE_POST_LINK += cp $$PWD/settings.json .
    QMAKE_CLEAN += settings.json
}

# Custom 'purge' target: clean build + delete QSettings + data folders for the current OS
purge.commands = $(MAKE) clean
macx {
    purge.commands += && defaults delete com.xmage.xmage-launcher-qt 2>/dev/null || true
    purge.commands += && rm -f ~/Library/Preferences/com.xmage.xmage-launcher-qt.plist
    purge.commands += && rm -rf ~/Library/Application\\ Support/xmage-launcher-qt/builds
    purge.commands += && rm -rf ~/Library/Application\\ Support/xmage-launcher-qt/decks
    purge.commands += && rm -rf ~/Library/Application\\ Support/xmage-launcher-qt/java
}
linux {
    purge.commands += && rm -f ~/.config/xmage/xmage-launcher-qt.conf
    purge.commands += && rmdir ~/.config/xmage 2>/dev/null || true
    purge.commands += && rm -rf builds decks java
}
win32 {
    purge.commands += && reg delete \"HKCU\\Software\\xmage\\xmage-launcher-qt\" /f 2>nul || true
    purge.commands += && rmdir /s /q builds decks java 2>nul || true
}
QMAKE_EXTRA_TARGETS += purge
