QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources/resources.qrc

macx {
    INCLUDEPATH += /opt/homebrew/opt/libzip/include
    LIBS += -L/opt/homebrew/opt/libzip/lib -lzip
    ICON = resources/icon-mage.icns

    # Automatically deploy Qt frameworks and sign the app bundle after build
    # Use -no-plugins to avoid errors from optional Qt modules, then manually copy needed plugins
    QMAKE_POST_LINK += macdeployqt $${TARGET}.app -no-plugins && \
        mkdir -p $${TARGET}.app/Contents/PlugIns/platforms && \
        mkdir -p $${TARGET}.app/Contents/PlugIns/styles && \
        mkdir -p $${TARGET}.app/Contents/PlugIns/imageformats && \
        cp -n /opt/homebrew/share/qt/plugins/platforms/libqcocoa.dylib $${TARGET}.app/Contents/PlugIns/platforms/ && \
        cp -n /opt/homebrew/share/qt/plugins/styles/libqmacstyle.dylib $${TARGET}.app/Contents/PlugIns/styles/ 2>/dev/null || true && \
        cp -n /opt/homebrew/share/qt/plugins/imageformats/libqjpeg.dylib $${TARGET}.app/Contents/PlugIns/imageformats/ && \
        macdeployqt $${TARGET}.app -no-plugins && \
        codesign --force --deep --sign - $${TARGET}.app

    # Clean up .app bundle on make clean
    QMAKE_CLEAN += -r $${TARGET}.app
}
linux {
    LIBS += -lzip
}
win32: LIBS += -lzip -lbz2 -llzma -lmsi
