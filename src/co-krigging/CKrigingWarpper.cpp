#include "CKrigingWarpper.h"
#include "CTriangulation.h"
#include <QDate>
#include <QTime>
#include <opencv/cv.h>

CKrigingWarpper::CKrigingWarpper()
{
    m_ppProjData = NULL;
}

CKrigingWarpper::CKrigingWarpper(CDTMParam paramDTM){
    m_paramDTM = paramDTM;
    m_ppProjData = NULL;
}

CKrigingWarpper::~CKrigingWarpper(){
    clear2DBuffer();
}

void CKrigingWarpper::clear2DBuffer(){
    if(m_ppProjData != NULL){
    for (int i = 0; i < m_paramDTM.m_ptSize.y; i++){
        if (m_ppProjData[i] != NULL) {
            delete [] m_ppProjData[i];
            m_ppProjData[i] = NULL;
        }
    }    
        delete [] m_ppProjData;
        m_ppProjData = NULL;
    }
}

void CKrigingWarpper::performInterpolation(const string strPly, const string strOut){

    tick();

    readData(strPly);// read ply
    prepareProjectionMap();//
    interpolation();// estimate

    toc();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // save heightmap, texturemap, and variance map
    string strSep = QString(QDir::separator()).toStdString();
    QString strTime = QDate::currentDate().toString("[DyyMMdd_T")+QTime::currentTime().toString("hhmmss]");
    string strResultDir = strOut + strSep + strTime.toStdString();

    QDir qdir;
    if (!qdir.exists(strResultDir.c_str())){
         qdir.mkpath(strResultDir.c_str());
    }

    string strRes = strResultDir + strSep + "DTM.txt";
    saveMatrix(m_matDTM, strRes);
    string strCp = strOut + strSep + "DTM.txt";
    cpFile(strRes, strCp);

    strRes = strResultDir + strSep+ "OrthoImg.txt";
    saveMatrix(m_matImg,strRes);
    strCp = strOut + strSep +  "OrthoImg.txt";
    cpFile(strRes, strCp);

    strRes = strResultDir + strSep + "DTMvar.txt";
    saveMatrix(m_matVarDTM, strRes);
//    strCp = strOut + strSep +  "DTMvar.txt";
//    cpFile(strRes, strCp);

    strRes = strResultDir + strSep + "OrthoVar.txt";
    saveMatrix(m_matVarImg, strRes);
//    strCp = strOut + strSep +  "OrthoVar.txt";
//    cpFile(strRes, strCp);

    strRes = strResultDir + strSep + FILE_LOG;
    saveDTMaram(m_paramDTM, strRes);

}

void CKrigingWarpper::readData(const string strPly){
    m_vecptXYZ.clear();
    m_vecptCol.clear();
    // if included .ply read as Ply file
    cout << "Reading input data" << endl;
    CTriangulation::loadPly(strPly, m_vecptXYZ, m_vecptCol);
    // otherwise general 3D data
}


void CKrigingWarpper::prepareProjectionMap(){
    cout << "preparing XY map" << endl;
    clear2DBuffer();
    // create buffer
    m_ppProjData = new CProjXY* [m_paramDTM.m_ptSize.y];
    for (int i = 0; i < m_paramDTM.m_ptSize.y; i++){
        m_ppProjData[i] = new CProjXY [m_paramDTM.m_ptSize.x];
    }

    // prepare data
    for (int i = 0; i < (int)m_vecptXYZ.size(); i++){
        int x = (int) round((m_vecptXYZ[i].x - m_paramDTM.m_ptStart.x)/ m_paramDTM.dRes);
        int y = (int) round((m_vecptXYZ[i].y - m_paramDTM.m_ptStart.y)/ m_paramDTM.dRes);

        if (x >= 0 && x < m_paramDTM.m_ptSize.x &&
            y >= 0 && y < m_paramDTM.m_ptSize.y){
            Point3i ptCol = m_vecptCol[i];
            m_ppProjData[y][x].m_vecptRGB.push_back(ptCol);
            Point3f ptXYZ = m_vecptXYZ[i];
            m_ppProjData[y][x].m_vecptXYZ.push_back(ptXYZ);
        }
    }

    // for debug
//    Mat matTemp = Mat::zeros(m_paramDTM.m_ptSize.y, m_paramDTM.m_ptSize.x, CV_32F);
//    for (int i = 0; i< m_paramDTM.m_ptSize.y; i++){
//        for (int j = 0; j < m_paramDTM.m_ptSize.x; j++){
//            CProjXY pr = m_ppProjData[i][j];
//            matTemp.at<float>(i,j) = pr.m_vecptRGB.size();
//        }
//    }
    //saveMatrix(matTemp, "/Users/DShin/Desktop/ts.txt");
    // debug
}

void CKrigingWarpper::interpolation(){
    cout << "interpolation" << endl;
//    int nNei = 7;
    // create output buffer
    m_matDTM = Mat::zeros(m_paramDTM.m_ptSize.y, m_paramDTM.m_ptSize.x, CV_64F);
    m_matImg = Mat::zeros(m_paramDTM.m_ptSize.y, m_paramDTM.m_ptSize.x, CV_64F);
    m_matVarDTM = Mat::zeros(m_paramDTM.m_ptSize.y, m_paramDTM.m_ptSize.x, CV_64F);
    m_matVarImg = Mat::zeros(m_paramDTM.m_ptSize.y, m_paramDTM.m_ptSize.x, CV_64F);

    for (int i = 0; i< m_paramDTM.m_ptSize.y; i++){
        for (int j = 0; j < m_paramDTM.m_ptSize.x; j++){

//            Point2f ptCentre;
//            ptCentre.x = m_paramDTM.m_ptStart.x + m_paramDTM.dRes*j;
//            ptCentre.y = m_paramDTM.m_ptStart.y + m_paramDTM.dRes*i;

            Point2i ptCentre(j,i);
            // getNeighbours
            vector<Point3f> vecNeiXYZ;
            vector<Point3i> vecNeiCol;
            getNeighbourData(ptCentre, vecNeiXYZ, vecNeiCol);
            int nNei = (int)vecNeiXYZ.size();
            if (nNei == 0) {
                // debug
                int x = i;
                int y = j;

                continue;
            }

            ////////////////////////////////////////////////////////////////
            // Kriging
            ////////////////////////////////////////////////////////////////
            PointK center(j,i);

            // The numbering of the nodes corresponds to gslib's output order
            Node* pNode = new Node [nNei];
            neighborhood voisin;
            for (int k = 0 ; k < nNei; k++){
                pNode->setData(vecNeiXYZ[k]);
                voisin.add_node(pNode[k]);
            }

            // get weighting coeff.
            typedef TNT_lib<double> TNT;
            typedef matrix_lib_traits<TNT>::Vector TNTvector;

            covariance covar;
            OK_constraints OK;
            double ok_variance;
            vector<double> weights;
            int status = kriging_weights(weights, ok_variance, center, voisin, covar, OK);

            // estimate new height
            double dEstZ = 0;
            for (int k = 0; k < nNei; k++){
                dEstZ += vecNeiXYZ[k].z * weights[k];
            }
            m_matDTM.at<double>(i,j) = dEstZ;
            m_matVarDTM.at<double>(i,j) = ok_variance;


            // estimate new colour
            neighborhood voisin2;
            for (int k = 0 ; k < nNei; k++){
                Point3f pt (vecNeiXYZ[k].x, vecNeiXYZ[k].y, vecNeiCol[k].x);
                pNode->setData(pt);
                voisin2.add_node(pNode[k]);
            }
            weights.clear();
            status = kriging_weights(weights, ok_variance, center, voisin2, covar, OK);
            double dEstCol = 0;
            for (int k = 0; k < nNei; k++){
                dEstCol += vecNeiCol[k].x * weights[k];
            }
            m_matImg.at<double>(i,j) = dEstCol;
            m_matVarImg.at<double>(i,j) = ok_variance;

            delete [] pNode;
        }
    }

    // apply median filter
    Mat matTemp(m_matDTM.rows, m_matDTM.cols, CV_32FC1);
    Mat matSrc(m_matDTM.rows, m_matDTM.cols, CV_32FC1);
    Mat matSrcInt(m_matDTM.rows, m_matDTM.cols, CV_32FC1);
    for (int i = 0; i < m_matDTM.rows; i++){
        for (int j= 0; j < m_matDTM.cols; j++){
            matSrc.at<float>(i,j) = m_matDTM.at<double>(i,j);
            matSrcInt.at<float>(i,j) = m_matImg.at<double>(i,j);
        }
    }

    medianBlur(matSrc, matTemp, 5);
    medianBlur(matTemp, matSrc, 5);
    medianBlur(matSrc, matTemp, 5);
    m_matDTM = matTemp.clone();

    matTemp = Mat::zeros(m_matDTM.rows, m_matDTM.cols, CV_32FC1);
    medianBlur(matSrcInt, matTemp, 5);
    medianBlur(matTemp, matSrcInt , 5);
    medianBlur(matSrcInt, matTemp , 5);
    m_matImg = matTemp.clone();
}

void  CKrigingWarpper::getNeighbourData(const Point2i& ptCentre, vector<Point3f>& vecNeiXYZ, vector<Point3i>& vecNeiCol){

    double distLimit = m_paramDTM.dRes * m_paramDTM.m_dDistLim;
    int nNeiLimit = m_paramDTM.m_nNeiLim;

    vecNeiXYZ.clear();
    vecNeiCol.clear();

    int nLen = m_ppProjData[ptCentre.y][ptCentre.x].m_vecptXYZ.size();
    Point2f ptIn;
    ptIn.x = ptCentre.x * m_paramDTM.dRes + m_paramDTM.m_ptStart.x;
    ptIn.y = ptCentre.y * m_paramDTM.dRes + m_paramDTM.m_ptStart.y;

    if (nLen == nNeiLimit){
        bool bRes = true;
//        for (int k = 0; k < nLen; k++){
//            double dist = m_ppProjData[ptCentre.y][ptCentre.x].getDistance(ptIn, k);
//            if (dist > distLimit) {
//                bRes = false;
//                break;
//            }
//        }
        if (bRes){
            vecNeiXYZ = m_ppProjData[ptCentre.y][ptCentre.x].m_vecptXYZ;
            vecNeiCol = m_ppProjData[ptCentre.y][ptCentre.x].m_vecptRGB;
        }
    }
    else if (nLen > nNeiLimit){ // find the best
        // get distance and sorted
        vector<double> vecDist;
        CProjXY projXY = m_ppProjData[ptCentre.y][ptCentre.x];
        for(int i = 0; i < nLen; i++){
            double dDist = projXY.getDistance(ptIn, i);
            vecDist.push_back(dDist);
        }
        sort(vecDist.begin(), vecDist.end());
        double distLim = vecDist[(int)max(nNeiLimit-1,0)];
        // collect best n
        for(int i = 0; i < nLen; i++){
            double dDist = projXY.getDistance(ptIn, i);
            if (dDist <= distLim){
                vecNeiXYZ.push_back(projXY.m_vecptXYZ[i]);
                vecNeiCol.push_back(projXY.m_vecptRGB[i]);
                if ((int)vecNeiXYZ.size() == nNeiLimit) break;
            }
        }
    }
    else{// if (nLen < nNeiLimit){ // add more from the neighbour

//        CProjXY projXY = m_ppProjData[ptCentre.y][ptCentre.x];
        //vecNeiXYZ.insert(vecNeiXYZ.end(), projXY.m_vecptXYZ.begin(), projXY.m_vecptXYZ.end());
        //vecNeiCol.insert(vecNeiCol.end(), projXY.m_vecptRGB.begin(), projXY.m_vecptRGB.end());
//        int nNeeded = nNeiLimit - vecNeiXYZ.size();
        int nWLimit  = (int)(distLimit / m_paramDTM.dRes);

        // collect all candidate
        vector<Point3f> vecTempXYZ;
        vector<Point3i> vecTempRGB;
        int nW = 1;
        bool bNeedMore = true;
        while (nW <= nWLimit && bNeedMore) {
            vector<Point2i> vecPtList;
            for (int j = ptCentre.y-nW; j < ptCentre.y+nW; j++){
                for (int i = ptCentre.x-nW; i < ptCentre.x+nW; i++){

                    for (int k = 0; k < (int)vecPtList.size(); k++){
                       if(vecPtList[i] == Point2i(i,j))
                           continue;
                    }

                    if (i >= 0 && i < m_paramDTM.m_ptSize.x &&
                        j >= 0 && j < m_paramDTM.m_ptSize.y &&
                        (int)m_ppProjData[j][i].m_vecptXYZ.size() > 0){
//                        CProjXY projData = m_ppProjData[j][i];
                        vecTempXYZ.insert(vecTempXYZ.end(),  m_ppProjData[j][i].m_vecptXYZ.begin(),  m_ppProjData[j][i].m_vecptXYZ.end());
                        vecTempRGB.insert(vecTempRGB.end(),  m_ppProjData[j][i].m_vecptRGB.begin(),  m_ppProjData[j][i].m_vecptRGB.end());
                        vecPtList.push_back(Point2i(i,j));
                    }
                }
            }
            if (vecTempXYZ.size() >= nNeiLimit)
                bNeedMore = false;
            nW++;
        }

        // select the best nNeeded
        vector<double> vecDist;
        int nLen = vecTempXYZ.size();
        if (nLen <= nNeiLimit) return;

        for(int i = 0; i < nLen; i++){
            Point2f ptTemp(vecTempXYZ[i].x, vecTempXYZ[i].y);
            double dDist = norm(ptIn - ptTemp);
            vecDist.push_back(dDist);
        }
        sort(vecDist.begin(), vecDist.end());
        double distLim = vecDist[(int)max(nNeiLimit-1,0)];

        if (distLim <= distLimit) {
            for(int i = 0; i < nLen; i++){
                Point2f ptTemp(vecTempXYZ[i].x, vecTempXYZ[i].y);
                double dDist = norm(ptIn - ptTemp);
                if (dDist <= distLim){
                    vecNeiXYZ.push_back(vecTempXYZ[i]);
                    vecNeiCol.push_back(vecTempRGB[i]);
                    if ((int)vecNeiXYZ.size() == nNeiLimit) break;
                }
            }
        }
    }
}
