TARGET = RegisterMap
TEMPLATE = app

QT       += core

SOURCES += main.cpp \
    CBatchProc.cpp

#LIBS += -L/usr/local/lib -lopencv_core
LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_features2d -lopencv_highgui -lopencv_photo
#INCLUDEPATH = /usr/local/include/
INCLUDEPATH += /usr/local/include/


HEADERS += \
    CBatchProc.h
