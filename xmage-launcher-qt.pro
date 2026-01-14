QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/aboutdialog.cpp \
    src/downloadmanager.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/settings.cpp \
    src/settingsdialog.cpp \
    src/unzipthread.cpp \
    src/xmageprocess.cpp

HEADERS += \
    src/aboutdialog.h \
    src/downloadmanager.h \
    src/mainwindow.h \
    src/settings.h \
    src/settingsdialog.h \
    src/unzipthread.h \
    src/xmageprocess.h

FORMS += \
    forms/mainwindow.ui \
    forms/settingsdialog.ui \
    forms/aboutdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources/resources.qrc

macx {
    INCLUDEPATH += /opt/homebrew/opt/libzip/include
    LIBS += -L/opt/homebrew/opt/libzip/lib -lzip
    # Deploy Qt frameworks and sign the app bundle
    QMAKE_POST_LINK += macdeployqt $${TARGET}.app && codesign --force --deep --sign - $${TARGET}.app
}
linux {
    LIBS += -lzip

    # AppImage target: run "make appimage" after building
    appimage.commands = $$PWD/packaging/linux/build-appimage.sh
    appimage.depends = $(TARGET)
    QMAKE_EXTRA_TARGETS += appimage
}
win32: LIBS += -lzip -lbz2 -llzma -lmsi
