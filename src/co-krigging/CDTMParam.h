#ifndef CDTMPARAM_H
#define CDTMPARAM_H

#include "opencv/cv.h"
using namespace cv;

class CDTMParam{
public:
    double dRes;           // spatial resolution in metre
    Point2d m_ptStart;     // starting position
    Point2i m_ptSize;      // width and height of the resulting DTM in pixel resolution

    int m_nNeiLim;
    double m_dDistLim;
};

#endif // CDTMPARAM_H
