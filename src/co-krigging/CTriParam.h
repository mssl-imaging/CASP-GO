#ifndef CTRIPARAM_H
#define CTRIPARAM_H

#include "CProjParam.h"
#include "CBAParam.h"

class CTriParam{
public:

    string getInputTP(){
        if (m_nTPType == TP_SIFT && !m_bInlierOnly) return "TP_SIFT";
        else if (m_nTPType == TP_SIFT_CSA && !m_bInlierOnly) return "TP_SIFT_CSA";
        else if (m_nTPType == TP_DOG_CSA && !m_bInlierOnly) return "TP_DOG_CSA";
        else if (m_nTPType == TP_SURF && !m_bInlierOnly) return "TP_SURF";
        else if (m_nTPType == TP_SURF_CSA && !m_bInlierOnly) return "TP_SURF_CSA";
        else if (m_nTPType == TP_DOH_CSA && !m_bInlierOnly) return "TP_DOH_CSA";
        else if (m_nTPType == TP_SIFT && m_bInlierOnly) return "TP_SIFT (inlier only)";
        else if (m_nTPType == TP_SIFT_CSA && m_bInlierOnly) return "TP_SIFT_CSA (inlier only)";
        else if (m_nTPType == TP_DOG_CSA && m_bInlierOnly) return "TP_DOG_CSA (inlier only)";
        else if (m_nTPType == TP_SURF && m_bInlierOnly) return "TP_SURF (inlier only)";
        else if (m_nTPType == TP_SURF_CSA && m_bInlierOnly) return "TP_SURF_CSA (inlier only)";
        else if (m_nTPType == TP_DOH_CSA && m_bInlierOnly) return "TP_DOH_CSA (inlier only)";
        else if (m_nTPType == TP_DENSE&& m_bInlierOnly) return "TP from the previous densification result (inlier only)";
        else if (m_nTPType == TP_DENSE&& !m_bInlierOnly) return "TP from the previous densification result";
        else return "TP_UNKNOWN";
    }

public:
    CTriParam():m_nTPType(TP_UNKNOWN)/*,m_nProcType(NO_FILTER)*/, m_bUseDispMap(false), m_bShow3D(true), m_b3DLim(false),
                m_fMinX(-10.f),m_fMaxX(10.f),m_fMinY(-10.f),m_fMaxY(10.f), m_fMinZ(-20.f),m_fMaxZ(20.f),
                m_bTin(false), m_bBA(false), m_bNonLin(false), m_fMaxDist(200.f), m_fMinDist(0.f),
                m_bInlierOnly(false), m_bUseUpdatedCamera(false){m_fMaxSim = 10000000000.f;}
    int m_nTPType;       // type of input points
    //int m_nProcType;   // preprocessing method

    bool m_bUseDispMap;
    bool m_b3DLim;
    float m_fMinX;
    float m_fMaxX;
    float m_fMinY;
    float m_fMaxY;

    float m_fMinZ;    // Z limit should be always used! (-20~20 or -20~40)
    float m_fMaxZ;

    bool m_bTin;
    bool m_bBA;
    bool m_bNonLin;   // use nonlinear calibration parameters: MER EDR data uses them
    float m_fMaxDist; // navcam upto 200m pancam 300m
    float m_fMinDist; //

    bool m_bInlierOnly;
    bool m_bUseUpdatedCamera;
    bool m_bShow3D;

    CProjParam m_paramProj;

    CBAParam m_paramBA;
    float m_fMaxSim;

    enum{TP_UNKNOWN, TP_SIFT, TP_SIFT_CSA, TP_DOG_CSA, TP_SURF, TP_SURF_CSA, TP_DOH_CSA, TP_DENSE};
   // enum{NO_FILTER, MED_FILTER};
    enum{NO_ERR, FILE_IO_ERR, FILE_CAM_ERR, FILE_TRI_ERR, FILE_BA_ERR};
};

#endif // CTRIPARAM_H
