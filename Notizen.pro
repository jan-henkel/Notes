#-------------------------------------------------
#
# Project created by QtCreator 2015-11-23T15:09:37
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32:RC_ICONS += icons/notepad.ico

TARGET = Notizen
TEMPLATE = app
CONFIG += c++11
CONFIG += -no-opengl
CONFIG += static

QT += printsupport

SOURCES += main.cpp\
        notizenmainwindow.cpp \
    notesinternals.cpp \
    notizentextedit.cpp \
    cryptointerface.cpp \
    passworddialog.cpp \
    settingsdialog.cpp \
    defaultvalues.cpp

HEADERS  += notizenmainwindow.h \
    notesinternals.h \
    notizentextedit.h \
    cryptointerface.h \
    passworddialog.h \
    settingsdialog.h \
    defaultvalues.h
FORMS    += notizenmainwindow.ui \
    passworddialog.ui \
    settingsdialog.ui

RESOURCES += \
    icons.qrc

TRANSLATIONS = localization/German.ts

win32:LIBS += -L$$PWD/cryptopp-install-windows/lib -lcryptopp
win32:INCLUDEPATH += $$PWD/cryptopp-install-windows/include
QMAKE_LFLAGS = -static
