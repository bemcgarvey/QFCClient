#-------------------------------------------------
#
# Project created by QtCreator 2015-07-18T18:54:14
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QFC
TEMPLATE = app


INCLUDEPATH += ../QFC32/src/

SOURCES += main.cpp\
        mainwindow.cpp \
    crc.cpp \
    aboutdialog.cpp \
    qhorizonwidget.cpp \
    picbootloader.cpp \
    hexfile.cpp \
    bootloaderthread.cpp \
    calibratedialog.cpp \
    qledwidget.cpp \
    quaternion.cpp \
    saveconfig.cpp

HEADERS  += mainwindow.h \
    crc.h \
    aboutdialog.h \
    qhorizonwidget.h \
    picbootloader.h \
    hexfile.h \
    bootloaderthread.h \
    calibratedialog.h \
    qledwidget.h \
    quaternion.h \
    saveconfig.h \
    ../QFC32/src/QFCConfig.h \
    ../QFC32/src/USBCommands.h

FORMS    += mainwindow.ui \
    aboutdialog.ui \
    calibratedialog.ui


RESOURCES += \
    resources.qrc

RC_FILE = qfc.rc

!CONFIG(static): {
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../QESHidUSB/release/ -lQESHidUSB
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../QESHidUSB/debug/ -lQESHidUSB
}

unix:CONFIG(release, debug|release): LIBS += -L$$PWD/../../QESHidUSB/lib-release/ -lQESHidUSB
unix:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../QESHidUSB/lib-debug/ -lQESHidUSB

INCLUDEPATH += $$PWD/../../QESHidUSB/Include
DEPENDPATH += $$PWD/../../QESHidUSB/Include

CONFIG(static): {
DEFINES += QESHIDUSB_STATIC_LIBRARY
win32: LIBS += -L$$PWD/../../../QESHidUSB/lib-static/ -lQESHidUSB
win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../QESHidUSB/lib-static/QESHidUSB.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../../QESHidUSB/lib-static/libQESHidUSB.a
win32: LIBS += -lhid
win32: LIBS += -lsetupapi
win32: LIBS += -luser32
}
