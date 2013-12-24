#-------------------------------------------------
#
# Project created by QtCreator 2013-12-23T15:53:43
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mandelbrotQt
TEMPLATE = app


SOURCES += main.cpp \
    mandelbrotwidget.cpp \
    renderthread.cpp \
    mandelbrotcl.cpp

HEADERS  += \
    mandelbrotwidget.h \
    renderthread.h \
    mandelbrotcl.h

INCLUDEPATH += "C:/Program Files (x86)/Intel/OpenCL SDK/3.0/include"

win32:LIBS += "C:/Program Files (x86)/Intel/OpenCL SDK/3.0/lib/x86/OpenCL.lib"
