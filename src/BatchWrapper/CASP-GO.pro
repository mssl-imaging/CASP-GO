TARGET = CASP-GO
TEMPLATE = app

QT       += core


QMAKE_CXXFLAGS += -std=c++0x

SOURCES += main.cpp \
    hiriseWrapper.cpp \
    ctxWrapper.cpp \
    CDensify.cpp \
    ALSC.cpp \
    CProcBlock.cpp \
    c2Wrapper.cpp

LIBS += -L/usr/opt/lib -lopencv_imgcodecs -lopencv_superres -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_photo -lopencv_features2d
INCLUDEPATH = /usr/opt/include/ \
    /usr/include/eigen3/Eigen/

HEADERS += \
    hiriseWrapper.h \
    ctxWrapper.h \
    CDensifyParam.h \
    CDensify.h \
    CGOTCHAParam.h \
    ALSC.h \
    CALSCParam.h \
    CProcBlock.h \
    CTiePt.h \
    c2Wrapper.h
