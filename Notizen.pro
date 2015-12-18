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
QT += printsupport

SOURCES += main.cpp\
        notizenmainwindow.cpp \
    notesinternals.cpp \
    cryptobuffer.cpp \
    notizentextedit.cpp

HEADERS  += notizenmainwindow.h \
    notesinternals.h \
    cryptobuffer.h \
    notizentextedit.h
FORMS    += notizenmainwindow.ui

RESOURCES += \
    icons.qrc

#TRANSLATIONS = en.ts \
#               ger.ts
