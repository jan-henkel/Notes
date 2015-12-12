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

SOURCES += main.cpp\
        notizenmainwindow.cpp \
    notesinternals.cpp \
    cryptobuffer.cpp

HEADERS  += notizenmainwindow.h \
    notesinternals.h \
    cryptobuffer.h
FORMS    += notizenmainwindow.ui

RESOURCES += \
    icons.qrc
