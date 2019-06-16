#ifndef CTRIANGULATION_H
#define CTRIANGULATION_H

#include "CTriParam.h"
#include "CProcBlock.h"
//#include

class CTriangulation : public CProcBlock
{
public:
    CTriangulation();
    CTriangulation(CTriParam paramTri);
    CTriangulation(CTriParam paramTri, vector<Point3f>& vecptVertex, vector<Point3i>& vecnColour);

    int performTriangulation(bool bSave = true);
    const vector<Point3f>& getPointCloud() const {return m_vecTri;}
    const vector<Point3i>& getPointColour() const {return m_vecCol;}
    const vector<Point2f> getImgPt(bool bIsLeft = true) const {
            vector<Point2f> vecPtRes;
            for (int i = 0; i < (int)m_vecUsed.size(); i++){
                int nIdx = m_vecUsed.at(i);
                CTiePt tp = m_vecTPs.at(nIdx);
                if (bIsLeft)
                    vecPtRes.push_back(tp.m_ptL);
                else
                    vecPtRes.push_back(tp.m_ptR);
            }
            return vecPtRes;
        }
    int getTotReconstruction() const {return m_vecTri.size();} // return get total number of reconstructed points        
    bool savePly(string strPly, bool bRef = false);

    static bool loadPly(const string strPly, vector<Point3f>& vecptVertex, vector<Point3i>& vecnColour);
    //static bool savePly(const string strOut, const vector<Point3f>& vecptTotVertex, const vector<Point3i>& vecnTotColour, bool bTIN);

private:
    //void getDLTfromCHAV(const Mat& matCAHV, Mat& matRes);
    bool LoadCamMat();
    bool reprojection();
    bool triangulation();
    Point3f getXYZ(const Point2f& ptL, const Point2f& ptR);       // used in triangulation
    Point2f getUndistortedPt(const Point2f& ptIn, bool bIsLeft);  // used in triangulation
    double getRaidalDistCoeff(const Point2f& ptIn, bool bIsLeft); // used in triangulation
    bool bundleAdjustment();
    bool isBehindCamera(const Point3f& ptIn);
    bool constructTIN();
    //bool constructTINfromDispMap();

    Point3i getColour(Point2f ptL);
    void setResultDir(string strProjDir);
    void setDispMapDir (string strProjDir);
    void setExtEXEPath(string strRscDir);

    // bool save3DptList();
    bool saveResult(); 
    bool saveLog();
    bool saveResLog(string strFile);
    bool SaveRefResult();

protected:
    // input parameters
    CTriParam m_paramTri;

    Mat m_matCamL;               // DLT camera matrix loaded from CALI folder
    Mat m_matCamR;
    Mat m_matCAHVOR_L;           // CAHVOR calibration parameters loaded from PDS folder
    Mat m_matCAHVOR_R;

    // output parameters
    Mat m_matCamLref;            // DLT camera model refinded by BA
    Mat m_matCamRref;    
    vector<Point3d> m_vecTriRef; // 3D points refined by BA
    vector<Point3f> m_vecTri;    // triangulated data points
    vector<Point3i> m_vecCol;    // colour data per each vertex
    vector<Point3i> m_vecTIN;    // TIN face data

    int m_nIter; // BA outputs
    Point2d m_ptErr;
    double m_dLambda;

private:
    // Temp parameters
    bool m_bTPready;
    string m_strResDir;          // result directory
    string m_strDispMapDir;
    string m_strExtEXEPathDT;      // need to know the path to the "resource"
    string m_strExtEXEPathML;
    vector<int> m_vecUsed;       // some tiepoint may not produce 3D points so that this variable records the index of a reconstructed point
};

#endif // CTRIANGULATION_H
