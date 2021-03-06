#-------------------------------------------------
#
# Project created by QtCreator 2016-05-11T10:12:45
#
#-------------------------------------------------

QT       -= gui

TARGET = QSavefile
TEMPLATE = lib

DEFINES += QSAVEFILE_LIBRARY

SOURCES += qsavefile.cpp

HEADERS += qsavefile.h\
        qsavefile_global.h

#ubuntu
unix:!macx{
LIBS        += -L/usr/local/linux_lib/lib -lQSlidingWindow -lQSlidingWindowConsume
INCLUDEPATH += ../QSlidingWindow
INCLUDEPATH += ../QSlidingWindowConsume
}
#arm
#unix:!macx {
#LIBS        += -L/usr/local/arm_lib -lQSlidingWindow -lQSlidingWindowConsume
#INCLUDEPATH += ../QSlidingWindow
#INCLUDEPATH += ../QSlidingWindowConsume
#}

#macx
unix:macx{
INCLUDEPATH += ../QSlidingWindow ../QSlidingWindowConsume
LIBS += -L/usr/local/lib -lQSlidingWindow -lQSlidingWindowConsume
}
#install
#mac
unix:macx {
    target.path = /usr/local/lib
    INSTALLS += target
}
#ubuntu
unix:!macx {
    target.path = /usr/local/linux_lib/lib
    INSTALLS += target
}
#arm
#unix:!macx {
#    target.path = /usr/local/arm_lib
#    INSTALLS += target
#}
