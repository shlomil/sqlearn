#-------------------------------------------------
#
# Project created by QtCreator 2012-11-05T01:00:33
#
#-------------------------------------------------

QT       += core gui sql

TARGET = sqlearn
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    sqleditor.cpp \
    simplesqlparser.cpp

HEADERS  += mainwindow.h \
    sqleditor.h \
    simplesqlparser.h

FORMS    += \
    sqleditor.ui

wince*: {
    DEPLOYMENT_PLUGIN += qsqlite
}
