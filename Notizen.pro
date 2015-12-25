#-------------------------------------------------
#
# Project created by QtCreator 2015-11-23T15:09:37
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Notizen
TEMPLATE = app
CONFIG += crypto
CONFIG += c++11
CONFIG += -no-opengl

QT += printsupport

SOURCES += main.cpp\
        notizenmainwindow.cpp \
    notesinternals.cpp \
    notizentextedit.cpp \
    cryptointerface.cpp \
    passworddialog.cpp \
    settingsdialog.cpp

HEADERS  += notizenmainwindow.h \
    notesinternals.h \
    notizentextedit.h \
    cryptointerface.h \
    passworddialog.h \
    settingsdialog.h
FORMS    += notizenmainwindow.ui \
    passworddialog.ui \
    settingsdialog.ui

RESOURCES += \
    icons.qrc

#TRANSLATIONS = en.ts \
#               ger.ts
