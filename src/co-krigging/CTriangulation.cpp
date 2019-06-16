#include "CTriangulation.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "CSBAnViews.h"

#include <QString>
//#include <QDir>
#include <QDate>
#include <QTime>

using namespace cv;
using namespace std;

CTriangulation::CTriangulation()
{
    m_strResDir = "qdelaunay";
    m_bTPready = false;
}

CTriangulation::CTriangulation(CTriParam paramTri) {
    m_paramTri = paramTri;
    setResultDir(paramTri.m_paramProj.m_strProjDir);
    setDispMapDir(paramTri.m_paramProj.m_strProjDir);
    if (!paramTri.m_bUseDispMap)
        setImages(paramTri.m_paramProj.m_strImgL, paramTri.m_paramProj.m_strImgR);
    else if (paramTri.m_bUseDispMap){
        string strTemp = paramTri.m_paramProj.m_strProjDir + paramTri.m_paramProj.m_strSep;
        string strImgL = strTemp + DIR_RECTIFY + paramTri.m_paramProj.m_strSep + FILE_IMG_RECT_L;
        string strImgR = strTemp + DIR_RECTIFY + paramTri.m_paramProj.m_strSep + FILE_IMG_RECT_R;
        setImages(strImgL, strImgR, false);
    }
    setExtEXEPath(paramTri.m_paramProj.m_strRsc);
    m_bTPready = false;
}

CTriangulation::CTriangulation(CTriParam paramTri, vector<Point3f>& vecptVertex, vector<Point3i>& vecnColour){
    m_paramTri = paramTri;

//    setResultDir(paramTri.m_paramProj.m_strProjDir);
    m_strResDir = paramTri.m_paramProj.m_strProjDir;
    setImages(paramTri.m_paramProj.m_strImgL, paramTri.m_paramProj.m_strImgR);
    setExtEXEPath(paramTri.m_paramProj.m_strRsc);

    m_vecTri = vecptVertex;    // triangulated data points
    m_vecCol = vecnColour;

}

void CTriangulation::setResultDir(string strProjDir){
    m_strResDir = strProjDir + m_paramTri.m_paramProj.m_strSep + DIR_TRI;
}

void CTriangulation::setDispMapDir(string strProjDir){
    m_strDispMapDir = strProjDir + m_paramTri.m_paramProj.m_strSep + DIR_DENSE;
}

void CTriangulation::setExtEXEPath(string strRscDir){
    m_strExtEXEPathDT = strRscDir + m_paramTri.m_paramProj.m_strSep + "qdelaunay";
    string strMLMacOS = "meshlab.app/Contents/MacOS/meshlab";
    m_strExtEXEPathML = strRscDir + m_paramTri.m_paramProj.m_strSep + strMLMacOS; //For MacOS temporarily run this to call meshlab, ("meshlab";) been replaced.
}

int CTriangulation::performTriangulation(bool bSave){

    // working order:
    // individual triangulation // removal constraints + non-linear correction
    // individual BA using strong Features only to update R/T between the camera and rover frame
    // update other trinagulation results based on the updated calibration parameters
    // save ply

    // load tps
    if (!m_paramTri.m_bUseDispMap){
        if (m_paramTri.m_bInlierOnly){
            // load inlier index
            string strIndxFile = m_paramTri.m_paramProj.m_strProjDir+ m_paramTri.m_paramProj.m_strSep+
                                 DIR_RECTIFY + m_paramTri.m_paramProj.m_strSep+ FILE_TP_INLIER_IDX;
            if (!FileExists(strIndxFile)) return CTriParam::FILE_IO_ERR;
            else{
                // read indx
                ifstream sfTPFile;
                sfTPFile.open(strIndxFile.c_str());
                int* pnIdx = NULL;
                int nTotLen = 0, nElement = 0;
                if (sfTPFile.is_open()){
                    sfTPFile >> nTotLen >> nElement;
                    pnIdx = new int [nTotLen];
                    for (int i = 0 ; i < nTotLen; i++){
                        sfTPFile >> pnIdx[i];
                    }
                }
                // read tp with idx
                if (!loadTP(m_paramTri.m_paramProj.m_strTPFile, pnIdx, nTotLen))
                    return CTriParam::FILE_IO_ERR;
            }
        }
        else{
            if (!loadTP(m_paramTri.m_paramProj.m_strTPFile)) // if (!loadInitialTiePts())
                return CTriParam::FILE_IO_ERR;
        }
        m_bTPready = true;
    }
    else if (m_paramTri.m_bUseDispMap){
        // load disparity map
        string strDispMapFile = m_strDispMapDir + m_paramTri.m_paramProj.m_strSep + FILE_DIS_MAP_X;
        if (!FileExists(strDispMapFile)) return CTriParam::FILE_IO_ERR;
        else{
            if (!loadMatrix(strDispMapFile))
                return CTriParam::FILE_IO_ERR;
        }
        m_bTPready = false;
    }



// ++++    m_bTPready = true;

    if (!LoadCamMat())
        return CTriParam::FILE_CAM_ERR;

    // load non-linear params
    if (true /*m_paramTri.m_bNonLin*/){

        // load left camera data
        string strCAHVOR = m_paramTri.m_paramProj.m_strProjDir+ m_paramTri.m_paramProj.m_strSep+
                           DIR_PDS + m_paramTri.m_paramProj.m_strSep+ DIR_PDS_LEFT + m_paramTri.m_paramProj.m_strSep + "CAHVOR.txt";
        m_matCAHVOR_L.create(6, 3, CV_32FC1);
        m_matCAHVOR_L = Mat::zeros(6, 3, CV_32FC1);
        loadMatrix(m_matCAHVOR_L,strCAHVOR);
//        if (!loadMatrix(m_matCAHVOR_L,strCAHVOR))
//            return CTriParam::FILE_IO_ERR;

        // load right camera data
        strCAHVOR = m_paramTri.m_paramProj.m_strProjDir+ m_paramTri.m_paramProj.m_strSep+
                    DIR_PDS + m_paramTri.m_paramProj.m_strSep+ DIR_PDS_RIGHT + m_paramTri.m_paramProj.m_strSep + "CAHVOR.txt";
        m_matCAHVOR_R.create(6, 3, CV_32FC1);
        m_matCAHVOR_R = Mat::zeros(6, 3, CV_32FC1);
        loadMatrix(m_matCAHVOR_R,strCAHVOR);
//        if (!loadMatrix(m_matCAHVOR_R,strCAHVOR))
//            return CTriParam::FILE_IO_ERR;
    }

    tick();
    // recontruction

    if (!m_paramTri.m_bUseDispMap){
        if (!triangulation())
            return CTriParam::FILE_TRI_ERR;
    }
    else if (m_paramTri.m_bUseDispMap){
        if (!reprojection())
            return CTriParam::FILE_TRI_ERR;
    }

    if(m_paramTri.m_bBA){
        if(!bundleAdjustment())
            return CTriParam::FILE_BA_ERR;
    }

    toc();

    if (bSave){
        // save result
        QString strTime = QDate::currentDate().toString("[DyyMMdd_T")+QTime::currentTime().toString("hhmmss]");
        m_strResDir = m_paramTri.m_paramProj.m_strProjDir+ m_paramTri.m_paramProj.m_strSep+
                DIR_TRI + m_paramTri.m_paramProj.m_strSep+ strTime.toStdString();
        QDir dirRect;
        if (!dirRect.exists(QString(m_strResDir.c_str()) ))
            dirRect.mkpath(QString(m_strResDir.c_str()));

        if(!saveResult())//Prepare result saving directories
            return CTriParam::FILE_IO_ERR;

        if(!saveLog())
            return CTriParam::FILE_IO_ERR;

        // copy the result to the parent folder
        string strFile = m_strResDir + m_paramTri.m_paramProj.m_strSep;
        string strOut =  m_paramTri.m_paramProj.m_strProjDir+ m_paramTri.m_paramProj.m_strSep+DIR_TRI+ m_paramTri.m_paramProj.m_strSep;
        string strIn = strFile+FILE_TRI_PLY;
        string strOutt = strOut+FILE_TRI_PLY;

        // delete the previous file first
        cpFile(strIn, strOutt);
        if (m_paramTri.m_bShow3D){
            if (!FileExists(m_strExtEXEPathML))
                return CTriParam::FILE_TRI_ERR;
            ostringstream strsCmd;
            strsCmd << m_strExtEXEPathML << " " << strOutt;

            system(strsCmd.str().c_str());
        }
    }
    return CTriParam::NO_ERR;
}

bool CTriangulation::reprojection(){
    bool bRes = true;
    if ( m_imgL.data == NULL || m_imgR.data == NULL)
        return CTriParam::FILE_IO_ERR;
    if ( m_dispX.data == NULL)
        return CTriParam::FILE_IO_ERR;
    m_vecTri.clear();
    m_vecCol.clear();
    Mat CamZhang = Mat::zeros(4,4,CV_32FC1);
    Mat Temp4Transform_1 = Mat::zeros(3,3,CV_32FC1);
    Temp4Transform_1.at<float>(0,0) = m_matCamL.at<float>(0,0);
    Temp4Transform_1.at<float>(0,1) = m_matCamL.at<float>(0,1);
    Temp4Transform_1.at<float>(0,2) = m_matCamL.at<float>(0,2);
    Temp4Transform_1.at<float>(1,0) = m_matCamL.at<float>(1,0);
    Temp4Transform_1.at<float>(1,1) = m_matCamL.at<float>(1,1);
    Temp4Transform_1.at<float>(1,2) = m_matCamL.at<float>(1,2);
    Temp4Transform_1.at<float>(2,0) = m_matCamL.at<float>(2,0);
    Temp4Transform_1.at<float>(2,1) = m_matCamL.at<float>(2,1);
    Temp4Transform_1.at<float>(2,2) = m_matCamL.at<float>(2,2);

    Mat Temp4Transform_2 = Mat::zeros(3,3,CV_32FC1);
    Temp4Transform_2.at<float>(0,0) = m_matCamR.at<float>(0,0);
    Temp4Transform_2.at<float>(0,1) = m_matCamR.at<float>(0,1);
    Temp4Transform_2.at<float>(0,2) = m_matCamR.at<float>(0,2);
    Temp4Transform_2.at<float>(1,0) = m_matCamR.at<float>(1,0);
    Temp4Transform_2.at<float>(1,1) = m_matCamR.at<float>(1,1);
    Temp4Transform_2.at<float>(1,2) = m_matCamR.at<float>(1,2);
    Temp4Transform_2.at<float>(2,0) = m_matCamR.at<float>(2,0);
    Temp4Transform_2.at<float>(2,1) = m_matCamR.at<float>(2,1);
    Temp4Transform_2.at<float>(2,2) = m_matCamR.at<float>(2,2);

    Mat Temp4Transform_3 = Mat::zeros(3,3,CV_32FC1);
    Mat Temp4Transform_4 = Mat::zeros(3,3,CV_32FC1);

    invert(Temp4Transform_1,Temp4Transform_3,DECOMP_SVD);
    invert(Temp4Transform_2,Temp4Transform_4,DECOMP_SVD);

    float A_1 = m_matCamL.at<float>(2,0)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));
    float A_2 = m_matCamL.at<float>(2,1)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));
    float A_3 = m_matCamL.at<float>(2,2)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));
    float H_1 = m_matCamL.at<float>(0,0)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));
    float H_2 = m_matCamL.at<float>(0,1)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));
    float H_3 = m_matCamL.at<float>(0,2)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));
    float V_1 = m_matCamL.at<float>(1,0)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));
    float V_2 = m_matCamL.at<float>(1,1)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));
    float V_3 = m_matCamL.at<float>(1,2)/(sqrt(m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2)));

    CamZhang.at<float>(0,0) = 1.;
    CamZhang.at<float>(0,1) = 0.;
    CamZhang.at<float>(0,2) = 0.;
    //-Cx Cx is the principal point x coordinate in the left image. Calculated from CAHV dot product of A and H (A_1*H_1+A_2*H_2+A_3*H_3) (CamL) or ((L1L9+L2L10+L3L11)/LL)
    CamZhang.at<float>(0,3) = -(A_1*H_1+A_2*H_2+A_3*H_3);
    CamZhang.at<float>(1,0) = 0.;
    CamZhang.at<float>(1,1) = 1.;
    CamZhang.at<float>(1,2) = 0.;
    //-Cy Cy is the principal point y coordinate in the left image. Calculated form CAHV dot product of A and V (A_1*V_1+A_2*V_2+A_3*V_3) (CamL) or ((L5L9+L6L10+L7L11)/LL)
    CamZhang.at<float>(1,3) = -(A_1*V_1+A_2*V_2+A_3*V_3);
    CamZhang.at<float>(2,0) = 0.;
    CamZhang.at<float>(2,1) = 0.;
    CamZhang.at<float>(2,2) = 0.;
    //Hs is usually taken as f to compute depth (recommended by JPL). Hs is calculated from CAHV ||cross product of A and H|| (CamL). Otherwise the average fx and fy could be used
    CamZhang.at<float>(2,3) = sqrt((A_2*H_3-A_3*H_2)*(A_2*H_3-A_3*H_2) + (A_3*H_1-A_1*H_3)*(A_3*H_1-A_1*H_3) + (A_1*H_2-A_2*H_1)*(A_1*H_2-A_2*H_1));
    CamZhang.at<float>(3,0) = 0.;
    CamZhang.at<float>(3,1) = 0.;
    //-1/Tx Tx is from the three translation parameters [Tx Ty Tz] which is the vector of the two cameras. Note that Tx calculated from OpenCV is a vector from Right camera to Left, but OpenCV assumes the Origin is
    //Left camera, therefore we should always add a minus to the T which is calculated from OpenCV. Here we take C_1 (CamR) - C_1 (CamL) as Tx
    CamZhang.at<float>(3,2) = -1./(-(Temp4Transform_4.at<float>(0,0)*m_matCamR.at<float>(0,3)+Temp4Transform_4.at<float>(0,1)*m_matCamR.at<float>(1,3)+Temp4Transform_4.at<float>(0,2)*1.)-
                                   (-(Temp4Transform_3.at<float>(0,0)*m_matCamL.at<float>(0,3)+Temp4Transform_3.at<float>(0,1)*m_matCamL.at<float>(1,3)+Temp4Transform_3.at<float>(0,2)*1.)));
    //(Cx-Cx')/Tx Cx' is the principal point x coordinate in the right image. (A_1*H_1+A_2*H_2+A_3*H_3) (CamR) or ((L1L9+L2L10+L3L11)/LL)
    CamZhang.at<float>(3,3) = ((m_matCamL.at<float>(0,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(0,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(0,2)*m_matCamL.at<float>(2,2))/
                               (m_matCamL.at<float>(2,0)*m_matCamL.at<float>(2,0)+m_matCamL.at<float>(2,1)*m_matCamL.at<float>(2,1)+m_matCamL.at<float>(2,2)*m_matCamL.at<float>(2,2))-
                               (m_matCamR.at<float>(0,0)*m_matCamR.at<float>(2,0)+m_matCamR.at<float>(0,1)*m_matCamR.at<float>(2,1)+m_matCamR.at<float>(0,2)*m_matCamR.at<float>(2,2))/
                               (m_matCamR.at<float>(2,0)*m_matCamR.at<float>(2,0)+m_matCamR.at<float>(2,1)*m_matCamR.at<float>(2,1)+m_matCamR.at<float>(2,2)*m_matCamR.at<float>(2,2)))/
                              (-(Temp4Transform_4.at<float>(0,0)*m_matCamR.at<float>(0,3)+Temp4Transform_4.at<float>(0,1)*m_matCamR.at<float>(1,3)+Temp4Transform_4.at<float>(0,2)*1.)-
                               (-(Temp4Transform_3.at<float>(0,0)*m_matCamL.at<float>(0,3)+Temp4Transform_3.at<float>(0,1)*m_matCamL.at<float>(1,3)+Temp4Transform_3.at<float>(0,2)*1.)));

    string strCamTranFile = m_paramTri.m_paramProj.m_strProjDir + m_paramTri.m_paramProj.m_strSep + DIR_CALI + m_paramTri.m_paramProj.m_strSep + FILE_CALI_T;
    bRes = bRes && saveMatrix(CamZhang, strCamTranFile);

    Mat rec3DL2R;
    reprojectImageTo3D(m_dispX, rec3DL2R, CamZhang, false);
    for (int i=0; i<rec3DL2R.rows; i++){
        for (int j=0; j<rec3DL2R.cols; j++){
            if (m_dispX.at<float>(i,j)!=0){
                Vec3f pointL2R = rec3DL2R.at<Vec3f>(i,j);
                Point3f pt3;
                pt3.x = pointL2R[0];
                pt3.y = pointL2R[1];
                pt3.z = pointL2R[2];
                m_vecTri.push_back(pt3);
                Point3i cBGR;
                cBGR.x = m_imgL.ptr<uchar>(i)[3*j+2];
                cBGR.y = m_imgL.ptr<uchar>(i)[3*j+1];
                cBGR.z = m_imgL.ptr<uchar>(i)[3*j+0];
                m_vecCol.push_back(cBGR);
            }
        }
    }
    if (m_paramTri.m_bTin)
    {
        bRes = bRes && constructTIN();
    }
    return bRes;
}


bool CTriangulation::bundleAdjustment(){

    CBAParam paramBA;

    vector<Mat> vmatCam;
    Mat matCamLd, matCamRd;
    m_matCamL.convertTo(matCamLd, CV_64FC1);
    m_matCamR.convertTo(matCamRd, CV_64FC1);
    vmatCam.push_back(matCamLd);
    vmatCam.push_back(matCamRd);

    //vptXw = m_vecTri; // copy
    int nPts = m_vecTri.size();
    int nVeiw = 2;
    Mat matXimg(nPts, nVeiw, CV_64FC2);
    vector<Point3d> vptXw;

    for (int i = 0; i < nPts; i++){
        Point3d pt3;
        pt3.x = (double)m_vecTri.at(i).x;
        pt3.y = (double)m_vecTri.at(i).y;
        pt3.z = (double)m_vecTri.at(i).z;
        vptXw.push_back(pt3);
        matXimg.at<Point2d>(i,0).x = (double)m_vecTPs.at(m_vecUsed.at(i)).m_ptL.x;
        matXimg.at<Point2d>(i,0).y = (double)m_vecTPs.at(m_vecUsed.at(i)).m_ptL.y;
        matXimg.at<Point2d>(i,1).x = (double)m_vecTPs.at(m_vecUsed.at(i)).m_ptR.x;
        matXimg.at<Point2d>(i,1).y = (double)m_vecTPs.at(m_vecUsed.at(i)).m_ptR.y;

    }

    CSBANViews sba(vmatCam, vptXw, matXimg, paramBA);
    if (!sba.solve()) return false;

    m_dLambda = sba.getLastLambda();
    m_nIter = sba.getNumIter();
    m_ptErr = sba.getErr();

    // collect results
//    saveMatrix(m_matCamL, "/Users/DShin/Desktop/l.txt");
//    saveMatrix(m_matCamL, "/Users/DShin/Desktop/r.txt");

    //
    m_matCamLref = sba.getUpdatedCam().at(0).clone();
    m_matCamRref = sba.getUpdatedCam().at(1).clone();
    m_vecTriRef = sba.getUpdated3D();

    return true;
}

bool CTriangulation::LoadCamMat(){

//    bool bRes = false;
    Mat matTemp;
    string strBase = m_paramTri.m_paramProj.m_strProjDir + m_paramTri.m_paramProj.m_strSep + DIR_CALI + m_paramTri.m_paramProj.m_strSep;
    string strCam;
    if(m_paramTri.m_bUseUpdatedCamera)
        strCam = strBase + FILE_CALI_L_UPDATED;
    else
        strCam = strBase + FILE_CALI_L;//m_paramTri.m_paramProj.m_strCalL;//strBase + FILE_CALI_L;

    m_matCAHVOR_L = Mat::zeros(6, 3, CV_32FC1);
    m_matCAHVOR_R = Mat::zeros(6, 3, CV_32FC1);

//    bRes = loadMatrix(matTemp, strCam);
    if (!loadMatrix(matTemp, strCam) || matTemp.rows * matTemp.cols != 12){
        cerr << "Left calibration file loading error" << endl;
        return false;
    }else{

        // debug
//            for (int i = 0; i < matTemp.rows; i++){
//                for (int j=0; j < matTemp.cols; j++){
//                    cout << matTemp.at<float> (i,j) << " ";
//                }
//                cout << endl;
//            }

        if (matTemp.rows == 3)
            m_matCamL = matTemp.clone(); // in case the input camera parameters from 12 DLT parameters
        else{
            getDLTfromCHAV(matTemp, m_matCamL); // when input parameters are from 4 thee dimensional vectors (i.e., CAHV)
            m_matCAHVOR_L.row(0) = matTemp.row(0);
            m_matCAHVOR_L.row(1) = matTemp.row(1);
            m_matCAHVOR_L.row(2) = matTemp.row(2);
            m_matCAHVOR_L.row(3) = matTemp.row(3);
        }
    }

    matTemp = Mat::zeros(1,1, CV_32F);
    if(m_paramTri.m_bUseUpdatedCamera)
        strCam = strBase + FILE_CALI_R_UPDATED;
    else
        strCam = strBase + FILE_CALI_R;//m_paramTri.m_paramProj.m_strCalL;//strBase + FILE_CALI_R;

    //strCam =  m_paramTri.m_paramProj.m_strCalR;//strBase + FILE_CALI_R;
//    bRes = loadMatrix(matTemp, strCam);
    if (!loadMatrix(matTemp, strCam) || matTemp.rows * matTemp.cols != 12){
         cerr << "Right calibration file loading error" << endl;
        return false;
    }else{
        if (matTemp.rows == 3)
            m_matCamR = matTemp.clone();
        else{
            getDLTfromCHAV(matTemp, m_matCamR);
            m_matCAHVOR_R.row(0) = matTemp.row(0);
            m_matCAHVOR_R.row(1) = matTemp.row(1);
            m_matCAHVOR_R.row(2) = matTemp.row(2);
            m_matCAHVOR_R.row(3) = matTemp.row(3);
        }
    }

    return true;
}

bool CTriangulation::isBehindCamera(const Point3f& ptIn){
    bool bRes = false;

    // get Plane equation from CAHV parameters
    float cx = m_matCAHVOR_L.at<float>(0,0);
    float cy = m_matCAHVOR_L.at<float>(0,1);
    float cz = m_matCAHVOR_L.at<float>(0,2);

    float nx = m_matCAHVOR_L.at<float>(1,0)-cx;
    float ny = m_matCAHVOR_L.at<float>(1,1)-cy;
    float nz = m_matCAHVOR_L.at<float>(1,2)-cz;

    if (cx == 0 && cy == 0 && cz == 0 &&
        nx == 0 && ny == 0 && nz == 0) // this is happening when camera parameters are obtained from the DLT -> no cahv or cahvor
        return false;

    //plane eq: nx(x-cx)+ny(y-cy)+nz(z-cz) = 0
    double dCost = nx*(ptIn.x-cx)+ny*(ptIn.y-cy)+nz*(ptIn.z-cz);
    if (dCost < 0 )
        bRes = true;
    return bRes;
}

bool CTriangulation::triangulation(){
    // point reconstruction
    bool bRes = true;
    m_vecTri.clear();
    m_vecCol.clear();
    //m_vecUsed.clear();

    int nLen = m_vecTPs.size();
    for (int i = 0; i < nLen; i++){
        if (m_vecTPs.at(i).m_ptL.x < 5 ||m_vecTPs.at(i).m_ptR.x < 5 ||
            m_vecTPs.at(i).m_ptL.y < 5 ||m_vecTPs.at(i).m_ptR.y < 5 ||
            m_vecTPs.at(i).m_ptL.x > m_imgL.cols-5 ||m_vecTPs.at(i).m_ptR.x > m_imgR.cols-5||
            m_vecTPs.at(i).m_ptL.y > m_imgL.rows-5 ||m_vecTPs.at(i).m_ptR.y > m_imgR.rows-5 )continue;

        Point3f pt3 = getXYZ(m_vecTPs.at(i).m_ptL, m_vecTPs.at(i).m_ptR);        
        float fMag = (float)sqrt(pt3.x*pt3.x + pt3.y*pt3.y + pt3.z*pt3.z);
        // Z range verify
        if (!(m_paramTri.m_fMinZ < pt3.z && pt3.z < m_paramTri.m_fMaxZ))
            continue;
        // distance verify
        if (!(m_paramTri.m_fMinDist < fMag && fMag < m_paramTri.m_fMaxDist)) continue;
        // check if a 3D point is behind the image plane
        if (isBehindCamera(pt3)) continue;

//        float a = m_vecTPs.at(i).m_fSimVal;
        if (m_vecTPs.at(i).m_fSimVal > m_paramTri.m_fMaxSim) continue;

        // range limit option
        if (m_paramTri.m_b3DLim){
            if( (m_paramTri.m_fMinX < pt3.x && pt3.x < m_paramTri.m_fMaxX) &&
                (m_paramTri.m_fMinY < pt3.y && pt3.y < m_paramTri.m_fMaxY) ){
                m_vecTri.push_back(pt3);
                m_vecCol.push_back(getColour(m_vecTPs.at(i).m_ptL));
                m_vecUsed.push_back(i);
            }
        }else{
         m_vecTri.push_back(pt3);
         m_vecCol.push_back(getColour(m_vecTPs.at(i).m_ptL));
         m_vecUsed.push_back(i);
        }
    }

    // mesh reconstruction
    if (m_paramTri.m_bTin)
    {
        bRes = bRes && constructTIN();
    }
    return bRes;
}


Point3f CTriangulation::getXYZ(const Point2f& ptLIn, const Point2f& ptRIn){

       Point3f ptRes;     

       // update PtL and PtR if there are non-linear params
       Point2f ptL;
       Point2f ptR;
       if (m_paramTri.m_bNonLin){
           ptL = getUndistortedPt(ptLIn, true);
           ptR = getUndistortedPt(ptRIn, false);

       }else{
           ptL = ptLIn;
           ptR = ptRIn;
       }

       // prepare a system matrix A
       Mat matA = Mat::zeros(4,4,CV_32F) ;
       matA.row(0) = (m_matCamL.row(2) * ptL.x) - (m_matCamL.row(0));
       matA.row(1) = (m_matCamL.row(2) * ptL.y) - (m_matCamL.row(1));
       matA.row(2) = (m_matCamR.row(2) * ptR.x) - (m_matCamR.row(0));
       matA.row(3) = (m_matCamR.row(2) * ptR.y) - (m_matCamR.row(1));

//       matA.setMatrix(0, 0, 0, 3, m_matDLTL.getMatrix(2, 2, 0, 3).times(ptLIn.x).minus(m_matDLTL.getMatrix(0, 0, 0, 3)));
//       matA.setMatrix(1, 1, 0, 3, m_matDLTL.getMatrix(2, 2, 0, 3).times(ptLIn.y).minus(m_matDLTL.getMatrix(1, 1, 0, 3)));
//       matA.setMatrix(2, 2, 0, 3, m_matDLTR.getMatrix(2, 2, 0, 3).times(ptRIn.x).minus(m_matDLTR.getMatrix(0, 0, 0, 3)));
//       matA.setMatrix(3, 3, 0, 3, m_matDLTR.getMatrix(2, 2, 0, 3).times(ptRIn.y).minus(m_matDLTR.getMatrix(1, 1, 0, 3)));

       // debug
//       for(int i = 0; i < matA.rows; i++){
//           for (int j = 0; j < matA.cols; j++)
//               cout << matA.at<float>(i,j) << " ";
//           cout << endl;
//       }


       // compute SVD of A
       Mat matRes(4,1,CV_32F);
       SVD::solveZ(matA, matRes);

       // debug
//       for(int i = 0; i < matRes.rows; i++){
//           for (int j = 0; j < matRes.cols; j++)
//               cout << matRes.at<float>(i,j) << " ";
//           cout << endl;
//       }


       matRes = matRes / matRes.at<float>(3,0);
       ptRes.x = matRes.at<float>(0,0);
       ptRes.y = matRes.at<float>(1,0);
       ptRes.z = matRes.at<float>(2,0);

       return ptRes;
}


Point2f CTriangulation::getUndistortedPt(const Point2f& ptIn, bool bIsLeft){


    Point2f ptRes;
//    Point2f ptCentre(0,0);
    double dCoeff = 0.f;
//    double r = 1.;
    bool bUpdate = false;

    if (bIsLeft){
        if(m_matCAHVOR_L.rows == 6 && m_matCAHVOR_L.cols == 3){ // is this really necessary?
            dCoeff = getRaidalDistCoeff(ptIn, bIsLeft); // dr/r
            bUpdate = true;
        }
    }
    else {
        if(m_matCAHVOR_R.rows == 6 && m_matCAHVOR_R.cols == 3){
            dCoeff = getRaidalDistCoeff(ptIn, bIsLeft); // dCoeff = dr/r
            bUpdate = true;
        }
    }

    // update new data
    if (bUpdate){
        ptRes.x = (ptIn.x)*dCoeff + ptIn.x;
        ptRes.y = (ptIn.y)*dCoeff + ptIn.y;
    }
    else ptRes = ptIn;

    return ptRes;
}


double CTriangulation::getRaidalDistCoeff(const Point2f& ptIn, bool bIsLeft){

    double dRes = 0.f;
    Mat matCAHVOR;
    if (bIsLeft) matCAHVOR = m_matCAHVOR_L;
    else matCAHVOR = m_matCAHVOR_R;

    Point2d ptCentre;
    // get centre
    ptCentre.x = matCAHVOR.at<float>(4,0)*matCAHVOR.at<float>(2,0)+
                 matCAHVOR.at<float>(4,1)*matCAHVOR.at<float>(2,1)+
                 matCAHVOR.at<float>(4,2)*matCAHVOR.at<float>(2,2);// hc = OH
    ptCentre.y = matCAHVOR.at<float>(4,0)*matCAHVOR.at<float>(3,0)+
                 matCAHVOR.at<float>(4,1)*matCAHVOR.at<float>(3,1)+
                 matCAHVOR.at<float>(4,2)*matCAHVOR.at<float>(3,2);//vc = OV

    // get radial distance
    Point2d ptNew;
    ptNew.x = ptIn.x-ptCentre.x;
    ptNew.y = ptIn.y-ptCentre.y;
    double r = norm(ptNew);

    // estimate dr/r
           // dr = k0*r + k1*r^3 + k2*r^5
           // k0 = m_matR(0,0)
           // k1 = m_matR(1,0)/fav^2
           // k2 = m_matR(2,0)/fav^4
           // fav = (fx+fy)/2
           // h = hs = fx = |O cross H|
           // v = vs = fy = |O cross V|

           double dr = 0.f;
           double k[3] = {0.,};
           double fav =1.f;
           double fx, fy;
           double h[3] = {0.,};
           double v[3] = {0.,};

           h[0] = matCAHVOR.at<float>(4,1) * matCAHVOR.at<float>(2,2) - matCAHVOR.at<float>(4,2) * matCAHVOR.at<float>(2,1);
           h[1] = matCAHVOR.at<float>(4,2) * matCAHVOR.at<float>(2,0) - matCAHVOR.at<float>(4,0) * matCAHVOR.at<float>(2,2);
           h[2] = matCAHVOR.at<float>(4,0) * matCAHVOR.at<float>(2,1) - matCAHVOR.at<float>(4,1) * matCAHVOR.at<float>(2,0);

           v[0] = matCAHVOR.at<float>(4,1) * matCAHVOR.at<float>(3,2) - matCAHVOR.at<float>(4,2) * matCAHVOR.at<float>(3,1);
           v[1] = matCAHVOR.at<float>(4,2) * matCAHVOR.at<float>(3,0) - matCAHVOR.at<float>(4,0) * matCAHVOR.at<float>(3,2);
           v[2] = matCAHVOR.at<float>(4,0) * matCAHVOR.at<float>(3,1) - matCAHVOR.at<float>(4,1) * matCAHVOR.at<float>(3,0);

           fx = sqrt(h[0]*h[0] + h[1]*h[1] + h[2]*h[2]);
           fy = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
           fav = (fx + fy) / 2.f;

           // get k coefficients
           k[0] = matCAHVOR.at<float>(5,0);
           k[1] = matCAHVOR.at<float>(5,1) /fav /fav;
           k[2] = matCAHVOR.at<float>(5,2) /fav /fav /fav /fav;
           dr = k[0]*r + k[1]*r*r*r +k[2]*r*r*r*r*r;

    dRes = dr/r;
    return dRes;
}


bool CTriangulation::constructTIN(){

    cout << "TIN construction" << endl;
    if (!FileExists(m_strExtEXEPathDT))
        return false;

    //create a temp TP list
    string strBase = m_strResDir + m_paramTri.m_paramProj.m_strSep;
    string strTP = strBase + FILE_TRI_TP_TEMP;
    ofstream fsTP;
    fsTP.open(strTP.c_str());
    if (fsTP.is_open()){
        if(m_bTPready){
            int nLen = m_vecUsed.size();
            fsTP << 2 << endl;
            fsTP << nLen << endl;
            for (int i = 0; i < nLen; i++){
                fsTP << m_vecTPs.at(m_vecUsed.at(i)).m_ptL.x << " " << m_vecTPs.at(m_vecUsed.at(i)).m_ptL.y << endl;
            }
            fsTP.close();
        }
        else{
            int nLen = m_vecTri.size();
            fsTP << 2 << endl;
            fsTP << nLen << endl;
            for (int i = 0; i < nLen; i++){
                fsTP << m_vecTri.at(i).x << " " << m_vecTri.at(i).y << endl;
            }
            fsTP.close();
        }
    }else
        return false;

    ostringstream strsCmd;
    string strOut = strBase + FILE_TRI_EDGE_LIST;
    strsCmd << m_strExtEXEPathDT << " Qt " << " i "
            << " < " << strTP << " > " << strOut;

    system(strsCmd.str().c_str());        

    if (!FileExists(strOut))
        return false;

    // collect data
    m_vecTIN.clear();
    ifstream ifTIN;
    ifTIN.open(strOut.c_str());
    if(ifTIN.is_open()){
        int nTot;
        ifTIN >> nTot;
        Point3i ptEdge;
        for (int i = 0; i < nTot; i++){
            ifTIN >> ptEdge.x >> ptEdge.y >> ptEdge.z;
            m_vecTIN.push_back(ptEdge);
        }
        ifTIN.close();
    }
    else
        return false;

    return true;
}

bool CTriangulation::saveResult(){

    bool bRes = true;
    //bRes = bRes && save3DptList();
    string strBase = m_strResDir + m_paramTri.m_paramProj.m_strSep;
    string strPly = strBase + FILE_TRI_PLY;
    bRes = bRes && savePly(strPly);
    if(m_paramTri.m_bBA){
        bRes = bRes && SaveRefResult();
    }

   return bRes;
}

bool CTriangulation::SaveRefResult(){
    bool bRes = true;
    string strBase = m_strResDir + m_paramTri.m_paramProj.m_strSep;

    // save cam mat
    string strFile = strBase + FILE_CALI_L_UPDATED;
    bRes = bRes && saveMatrix(m_matCamLref, strFile);

    strFile = strBase + FILE_CALI_R_UPDATED;
    bRes = bRes && saveMatrix(m_matCamRref, strFile);

    // save new ply
    strFile = strBase + FILE_PLY_UPDATED;
    bRes = bRes && savePly(strFile, true);
    return bRes;
}

bool CTriangulation::loadPly(const string strPly, vector<Point3f>& vecptVertex, vector<Point3i>& vecnColour){
    bool bRes = false;

    ifstream fsPly;
    fsPly.open(strPly.c_str());
    if(fsPly.is_open()){
        string strIn;
        fsPly >> strIn;
        if (strIn.compare("ply") == 0){
            do {
                fsPly >> strIn;
            } while(strIn.compare("vertex") != 0 && !fsPly.eof());

            if (!fsPly.eof()) {
                int nTotVer = 0 ;
                fsPly >> nTotVer;

                do {
                    fsPly >> strIn;
                } while(strIn.compare("end_header") != 0 && !fsPly.eof());
                if (!fsPly.eof()){
                    for (int i = 0 ; i < nTotVer; i++){
                        Point3f ptVer;
                        Point3i ptInt;
                        fsPly >> ptVer.x >> ptVer.y >> ptVer.z >> ptInt.x >> ptInt.y >> ptInt.z;

                        vecptVertex.push_back(ptVer);
                        vecnColour.push_back(ptInt);
                    }
                    bRes = true;
                }
            }
        }
    }

    return bRes;
}

bool CTriangulation::savePly(string strPly, bool bRef){
    // ply file
    //    string strBase = m_strResDir + m_paramTri.m_paramProj.m_strSep;
//    string strPly = strBase + FILE_TRI_PLY;
    ofstream fsPly;
    fsPly.open(strPly.c_str());
    if (fsPly.is_open()){
         int nTotPt = m_vecTri.size();
         int nTotFace = m_vecTIN.size();

        fsPly << "ply" << endl;
        fsPly << "format ascii 1.0" << endl;
        fsPly << "element vertex " << nTotPt << endl;
        fsPly << "property float32 x" << endl;
        fsPly << "property float32 y" << endl;
        fsPly << "property float32 z" << endl;
        fsPly << "property uint8 red" << endl;
        fsPly << "property uint8 green" << endl;
        fsPly << "property uint8 blue" << endl;
        if (m_paramTri.m_bTin) {
            fsPly << "element face " << nTotFace << endl;
            fsPly << "property list uint8 int32 vertex_indices" << endl;
        }
        fsPly << "end_header" << endl;

        // write vertecies
        for (int i = 0; i < nTotPt; i++) {
            Point3f pt3;
            if (bRef)
                pt3 = m_vecTriRef.at(i);
            else
                pt3 = m_vecTri.at(i);

            Point3i col = m_vecCol.at(i);
            fsPly << pt3.x << " " << pt3.y << " " << pt3.z << " "
                  << col.x << " " << col.y << " " << col.z << endl;
        }

        // write faces
        if (m_paramTri.m_bTin) {
            for (int i = 0; i < nTotFace; i++) {
                fsPly << 3 << " " << m_vecTIN.at(i).x <<" "<< m_vecTIN.at(i).y <<" "<< m_vecTIN.at(i).z <<endl;
            }
        }
        fsPly.close();
    }
    else
        return false;

    return true;
}


Point3i CTriangulation::getColour(Point2f ptL){
    int nX = (int)floor(ptL.x);
    int nY = (int)floor(ptL.y);
    //ttttt int nRes = m_imgL.at<uchar>(nY,nX);
    //Mat ttttt = imread("ttttt.tif",1);
    //ttttt.convertTo(ttttt, CV_8UC3);

    Point3i ptRes;
    if (m_imgL.channels() == 3){
        int nRes_R = m_imgL.ptr<uchar>(nY)[3*nX+2];
        int nRes_G = m_imgL.ptr<uchar>(nY)[3*nX+1];
        int nRes_B = m_imgL.ptr<uchar>(nY)[3*nX+0];
        ptRes = Point3i(nRes_R,nRes_G,nRes_B);
    }
    else if (m_imgL.channels() == 1){
        int nRes = m_imgL.at<uchar>(nY,nX);
        ptRes = Point3i(nRes,nRes,nRes);
    }
    else
        cout << "ERROR READ IN MATRIX! DEBUG SEE CTriangulation::getColour"<< endl;

    return ptRes;
}

bool CTriangulation::saveLog(){
    string strFile = m_strResDir + m_paramTri.m_paramProj.m_strSep + FILE_LOG;
    // save processing param
    bool bRes = true;
    bRes = bRes && saveTriParam(m_paramTri, strFile);
    bRes = bRes && saveBAParam(m_paramTri.m_paramBA, strFile);

    // result summary
    bRes = bRes && saveResLog(strFile);

    return bRes;
}

bool CTriangulation::saveResLog(string strFile){
    bool bRes = false;

    ofstream sfLog;
    sfLog.open(strFile.c_str(), ios::app | ios::out);

    if (sfLog.is_open()){
        sfLog << "<Processing results>" << endl;
        sfLog << "Input TP type: " << m_paramTri.getInputTP() << endl;
        sfLog << "Total number of input TPs: " << getNumTps() << endl;
        sfLog << "Total number of 3D points: " << m_vecTri.size() << endl;
        sfLog << "Total number of faces: "  << m_vecTIN.size() << endl;
        sfLog << "Total processing time [sec]: " << getProcTime() << endl;
        if (m_paramTri.m_bBA){
            sfLog << "Total BA iterations: " << m_nIter << endl;
            sfLog << "Final lambda: " << m_dLambda << endl;
            sfLog << "Initial error: " << m_ptErr.x << endl;
            sfLog << "Refined error: " << m_ptErr.y << endl;
        }

        sfLog.close();
        bRes = true;
    }
    else
        return bRes = false;

    return bRes;
}
