TARGET = GotchaDisparityRefine
TEMPLATE = app

QT       += core
QMAKE_CXXFLAGS += -std=c++0x
SOURCES += main.cpp \
    CBatchProc.cpp \
    CDensify.cpp \
    CProcBlock.cpp \
    ALSC.cpp \

HEADERS  += CBatchProc.h \
    CALSCParam.h \
    CTiePt.h \
    CDensifyParam.h \
    CProcBlock.h \
    CGOTCHAParam.h \
    CDensify.h \
    ALSC.h \

LIBS += -L/usr/lib -lopencv_imgcodecs -lopencv_superres -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_photo -lopencv_features2d
INCLUDEPATH = /usr/include/ \
    /usr/include/eigen3/Eigen/
