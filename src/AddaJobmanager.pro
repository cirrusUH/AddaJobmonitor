#-------------------------------------------------
#
# Project created by QtCreator 2016-12-21T12:00:29
#
#-------------------------------------------------

QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AddaJobmanager
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qssh.cpp \
    qcustomplot.cpp \
    helpabout.cpp \
    jal.cpp

HEADERS  += mainwindow.h \
    qssh.h \
    qcustomplot.h \
    helpabout.h \
    jal.h

FORMS    += mainwindow.ui \
    helpabout.ui

QMAKE_CXXFLAGS += -std=c++1y
