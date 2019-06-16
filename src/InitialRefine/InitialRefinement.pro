QT       += core
QT       -= gui

TARGET = InitialRefinement
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp
INCLUDEPATH += /usr/local/include/

LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_highgui -lopencv_ml -lopencv_calib3d -lopencv_photo
