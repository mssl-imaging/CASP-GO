#include <fstream>
#include <iostream>
#include <math.h>
#include "CDensify.h"
#include "ALSC.h"


using namespace std;

CDensify::CDensify()
{
}

CDensify::CDensify(CDensifyParam paramDense){
    setParameters(paramDense);
}

void CDensify::setParameters(CDensifyParam paramDense){
    m_paramDense = paramDense;
    loadImages();
}


void CDensify::loadImages(){
    string strImgL, strImgR;
    // get input images
    strImgL = m_paramDense.m_strImgL;
    strImgR = m_paramDense.m_strImgR;
    setImages(strImgL, strImgR);

}


bool CDensify::loadTPForDensification(string strTPFile){
    bool bRes = true;
    bRes = loadTP(strTPFile);
    return bRes;
}

int CDensify::performDensitification(){

    ///////////////////////////////////////////
    // validate the input images
    ///////////////////////////////////////////
    if ( m_imgL.data == NULL || m_imgR.data == NULL)
        return CDensifyParam::FILE_IO_ERR;

    if (!loadTPForDensification(m_paramDense.m_strTPFile))
        return CDensifyParam::FILE_IO_ERR;

    if(m_paramDense.m_nProcType == CDensifyParam::GOTCHA){
        double start = getTickCount();
        // Generate integer-float seed points (nb. feature detection only gives float-float seed points)
        // Also, this is necessary to keep the original seed data, as initial ALSC will remove some seeds if selected.

        //Function below run initial ALSC on disparity based TPs. If use, need to change:
        //1. doGotcha(m_imgL, m_imgR, vecSeedTemp, m_paramDense.m_paramGotcha, m_vectpAdded)
        //2. m_vectpAdded.insert(m_vectpAdded.end(), vecSeedTemp.begin(), vecSeedTemp.end());
        //3. need some change in doPGotcha

        vector<CTiePt> vecSeedTemp = getIntToFloatSeed(m_vecTPs);
        m_vecTPs.clear();
        for (int j = 0; j < (int)vecSeedTemp.size(); j++){
            CTiePt tpTemp = vecSeedTemp.at(j);
            m_vecTPs.push_back(tpTemp);
        }

        cout << "*  MAGiGAN: starting Gotcha ...                                         *" << endl;

        if (!doGotcha(m_imgL, m_imgR, m_vecTPs, m_paramDense.m_paramGotcha, m_vectpAdded))
            return CDensifyParam::GOTCHA_ERR;

        double dYLimit = 100;
        vector<CTiePt>::iterator iter;
        for (iter = m_vectpAdded.begin(); iter < m_vectpAdded.end(); ){
            double dy = iter->m_ptL.y - iter->m_ptR.y;
            dy = sqrt(dy*dy);
            if (dy > dYLimit)
                m_vectpAdded.erase(iter);
            else
                iter++;
        }

        m_vectpAdded.insert(m_vectpAdded.end(), m_vecTPs.begin(), m_vecTPs.end());

        double end = getTickCount();
        procTime = (end - start)/getTickFrequency();

        cout << "*  MAGiGAN: making data products ...                                    *" << endl;

        makeDataProducts();

        if (!saveResult())
            return CDensifyParam::FILE_IO_ERR;

        if (!saveLog())
            return CDensifyParam::FILE_IO_ERR;

    }
    else if (m_paramDense.m_nProcType == CDensifyParam::P_GOTCHA){
        double start = getTickCount();
        if (!doPGotcha(m_paramDense.m_paramGotcha.m_nNeiType))
            return CDensifyParam::P_GOTCHA_ERR;
        double dYLimit = 100;
        vector<CTiePt>::iterator iter;
        for (iter = m_vectpAdded.begin(); iter < m_vectpAdded.end(); ){
            double dy = iter->m_ptL.y - iter->m_ptR.y;
            dy = sqrt(dy*dy);
            if (dy > dYLimit)
                m_vectpAdded.erase(iter);
            else
                iter++;
        }

        m_vectpAdded.insert(m_vectpAdded.end(), m_vecTPs.begin(), m_vecTPs.end());

        double end = getTickCount();
        procTime = (end - start)/getTickFrequency();
        makeDataProducts();

        if (!saveResult())
            return CDensifyParam::FILE_IO_ERR;

        if (!saveLog())
            return CDensifyParam::FILE_IO_ERR;
    }

    return CDensifyParam::NO_ERR;
}

vector<CTiePt> CDensify::getIntToFloatSeed(vector<CTiePt>& vecTPSrc) {
    vector<CTiePt> vecRes;

    int nLen =  vecTPSrc.size();
    for (int i = 0; i < nLen; i++){
        CTiePt tp = vecTPSrc.at(i);
        Point2f ptL = tp.m_ptL;
        Point2f ptR = tp.m_ptR;
        float dX = 0, dY = 0;

        dX = floor(ptL.x) - ptL.x;
        dY = floor(ptL.y) - ptL.y;
        if (dX == 0 && dY == 0){
            vecRes.push_back(tp);
            continue;
        }

        Point2f ptDelta(dX,dY);
        Point2f ptIntL = ptL + ptDelta;
        Point2f ptIntR = ptR + ptDelta;

        /////////////////////////////////////////////////////
        // collect 4 integer seed and validate
        /////////////////////////////////////////////////////
        CTiePt tpTemp = tp;
        vector<CTiePt> vectpSeeds;
        // pt1 (top-left)
        tpTemp.m_ptL = ptIntL;
        tpTemp.m_ptR = ptIntR;
        vectpSeeds.push_back(tpTemp);
        // pt2 (top-right)
        tpTemp.m_ptL = ptIntL + Point2f(1, 0);
        tpTemp.m_ptR = ptIntR + Point2f(1, 0);
        vectpSeeds.push_back(tpTemp);

        // pt3 (bottom-left)
        tpTemp.m_ptL = ptIntL + Point2f(0, 1);
        tpTemp.m_ptR = ptIntR + Point2f(0, 1);
        vectpSeeds.push_back(tpTemp);

        // pt 4 (bottom-right)
        tpTemp.m_ptL = ptIntL + Point2f(1, 1);
        tpTemp.m_ptR = ptIntR + Point2f(1, 1);
        vectpSeeds.push_back(tpTemp);

        //apply ALSC to collect as new seed
        ALSC alsc(m_imgL, m_imgR,  m_paramDense.m_paramGotcha.m_paramALSC);
        alsc.performALSC(&vectpSeeds);
        vectpSeeds.clear();
        alsc.getRefinedTps(vectpSeeds); // hard-copy

        for (int j = 0; j < (int)vectpSeeds.size(); j++){
            CTiePt tpRef = vectpSeeds.at(j);
            if (tpRef.m_fSimVal != CTiePt::NOT_DEF){
                vecRes.push_back(tpRef);
            }
        }
    }

    return vecRes;
}

bool CDensify::saveProjLog (string strFile){
    bool bRes = false;
    ofstream sfLog;
    sfLog.open(strFile.c_str(), ios::app | ios::out);

    if (sfLog.is_open()){
        sfLog << "<Project I/O>" << endl;
        sfLog << "Input LEFT image path: " << m_paramDense.m_strImgL << endl;
        sfLog << "Input RIGHT image path: " << m_paramDense.m_strImgR << endl;
        sfLog << "Output x disparity map path: " << m_paramDense.m_strUpdatedDispX << endl;
        sfLog << "Output y disparity map path: " << m_paramDense.m_strUpdatedDispY << endl;
        sfLog << endl;
        sfLog.close();
        bRes = true;
    }
    else
        return bRes = false;

    return bRes;
}

bool CDensify::saveLog(){

    string FILE_LOG = "-GLog.txt";
    string strFile = m_paramDense.m_strOutPath + FILE_LOG;
    bool bRes = false;

    // save parameter log
    bRes = saveProjLog(strFile);
    bRes = saveGOTCHAParam(m_paramDense.m_paramGotcha, strFile);
    bRes = bRes && saveALSCParam(m_paramDense.m_paramGotcha.m_paramALSC, strFile);
    // save result log
    bRes = bRes && saveResLog(strFile);

    return bRes;
}

bool CDensify::saveResLog(string strFile){

    bool bRes = false;
    int nNumFinalTPs = nNumSeedTPs+m_vectpAdded.size();
    ofstream sfLog;
    sfLog.open(strFile.c_str(), ios::app | ios::out);

    if (sfLog.is_open()){
        sfLog << "<Processing results>" << endl;
        sfLog << "Processing method: " << m_paramDense.getProcessingType() << endl;
        sfLog << "Total number of seed TPs: " << nNumSeedTPs << endl;
        sfLog << "Total number of final TPs: " << nNumFinalTPs << endl;
        sfLog << "Total processing time(sec): " << procTime << endl;
        sfLog << endl;
        sfLog << endl;
        sfLog.close();
        bRes = true;
    }
    else
        return bRes = false;

    return bRes;

}

void CDensify::makeDataProducts(){

    // prepare output data product from the list of densified tiepoints
    // make output products (disparity map x,y and sim)
    int nW = m_imgL.cols;
    int nH = m_imgL.rows;

    // clear&initialise output buffers
    m_matDisMapX = Mat::ones(nH, nW, CV_32FC1)*0.0;
    m_matDisMapY = Mat::ones(nH, nW, CV_32FC1)*0.0;
    m_matDisMapSim = Mat::ones(nH, nW, CV_32FC1)*-1;

    // fill the disparity map
    int nLen = m_vectpAdded.size();
    for (int i = 0 ; i < nLen; i++){
        CTiePt tp = m_vectpAdded.at(i);
        Point2f ptL = tp.m_ptL;
        Point2f ptR = tp.m_ptR;
        float fSim = tp.m_fSimVal;

        int x = (int) ptL.x;
        int y = (int) ptL.y;
        Rect_<int> rect(0,0,nW,nH);
        if (rect.contains(Point2i(x,y))) {
            m_matDisMapX.at<float>(y,x) = ptR.x - x;
            m_matDisMapY.at<float>(y,x) = ptR.y - y;
            m_matDisMapSim.at<float>(y,x) = fSim;
        }
    }
}


bool CDensify::saveResult(){

    bool bRes = true;

    cout << "*                                                                       *" << endl;
    cout << "*  MAGiGAN: writing Gotcha results ...                                  *" << endl;
    cout << "*                                                                       *" << endl;

    Mat dispX = Mat::ones(m_matDisMapX.size(), CV_32FC1)*0.0;
    Mat dispY = Mat::ones(m_matDisMapY.size(), CV_32FC1)*0.0;

    for (int i =0; i<dispX.rows; i++){
        for (int j=0; j<dispX.cols; j++){
            if (m_matDisMapX.at<float>(i,j)!= 0.0){ //-3.40282346639e+038
                float disp = m_matDisMapX.at<float>(i,j);
                dispX.at<float>(i,j)=disp;
            }
        }
    }

    for (int i =0; i<dispY.rows; i++){
        for (int j=0; j<dispY.cols; j++){
            if (m_matDisMapY.at<float>(i,j)!= 0.0){
                float disp = m_matDisMapY.at<float>(i,j);
                dispY.at<float>(i,j)=disp;
            }
        }
    }

    string strFileC1 = m_paramDense.m_strUpdatedDispX;
    bRes = bRes && saveMatrix(dispX, strFileC1);
    string strFileC2 = m_paramDense.m_strUpdatedDispY;
    bRes = bRes && saveMatrix(dispY, strFileC2);

    return bRes;
}

void CDensify::getNeighbour(const CTiePt tp, vector<CTiePt>& vecNeiTp, const int nNeiType, const Mat& matSim){
    //
    Point2f ptLeft = tp.m_ptL;
    Point2f ptRight = tp.m_ptR;

    if (nNeiType == CGOTCHAParam::NEI_DIFF){
        getDisffusedNei(vecNeiTp, tp, matSim);
    }

    else {
        if (nNeiType == CGOTCHAParam::NEI_4  || nNeiType == CGOTCHAParam::NEI_Y || nNeiType == CGOTCHAParam::NEI_8){
            CTiePt tp2;
            tp2.m_ptL = ptLeft + Point2f(0,1);
            tp2.m_ptR = ptRight + Point2f(0,1);
            vecNeiTp.push_back(tp2);
            tp2.m_ptL = ptLeft + Point2f(0,-1);
            tp2.m_ptR = ptRight + Point2f(0,-1);
            vecNeiTp.push_back(tp2);
        }

        if (nNeiType == CGOTCHAParam::NEI_X || nNeiType == CGOTCHAParam::NEI_4 || nNeiType == CGOTCHAParam::NEI_8){

            CTiePt tp;
            tp.m_ptL = ptLeft + Point2f(1,0);
            tp.m_ptR = ptRight + Point2f(1,0);
            vecNeiTp.push_back(tp);
            tp.m_ptL = ptLeft + Point2f(-1,0);
            tp.m_ptR = ptRight + Point2f(-1,0);
            vecNeiTp.push_back(tp);
        }

        if (nNeiType == CGOTCHAParam::NEI_8){
            CTiePt tp8;
            tp8.m_ptL = ptLeft + Point2f(-1,-1);
            tp8.m_ptR = ptRight + Point2f(-1,-1);
            vecNeiTp.push_back(tp8);
            tp8.m_ptL = ptLeft + Point2f(1,1);
            tp8.m_ptR = ptRight + Point2f(1,1);
            vecNeiTp.push_back(tp8);
            tp8.m_ptL = ptLeft + Point2f(-1,1);
            tp8.m_ptR = ptRight + Point2f(-1,1);
            vecNeiTp.push_back(tp8);
            tp8.m_ptL = ptLeft + Point2f(1,-1);
            tp8.m_ptR = ptRight + Point2f(1,-1);
            vecNeiTp.push_back(tp8);
        }
    }
}

void CDensify::getDisffusedNei(vector<CTiePt>& vecNeiTp, const CTiePt tp, const Mat& matSim){
    // estimate the growth
    // make a diffusion map
    int nSzDiff = m_paramDense.m_paramGotcha.m_paramALSC.m_nPatch;//12
    double pdDiffMap[nSzDiff*2+1][nSzDiff*2+1];
    int nW = m_imgL.cols;
    int nH = m_imgR.rows;
    Rect rectRegion(0,0,nW, nH);

    Point2f ptLeft = tp.m_ptL;
    Point2f ptRight = tp.m_ptR;
    for (int j = -nSzDiff ; j < nSzDiff + 1; j++){
        for (int i = -nSzDiff ; i < nSzDiff + 1; i++){
            double val = 0.f;
            int nX = (int)floor(ptLeft.x+i);
            int nY = (int)floor(ptLeft.y+j);
            if (rectRegion.contains(Point(nX, nY)) &&
                matSim.at<float>(nY, nX) > 0){
                // assume sim value has been already normalised
                val = 1.f - matSim.at<float>(nY, nX);// /m_paramDense.m_paramGotcha.m_paramALSC.m_fEigThr;
            }
            pdDiffMap[j+nSzDiff][i+nSzDiff] = val;
        }
    }

    // heat diffusion equation
    double dAlpha = m_paramDense.m_paramGotcha.m_fDiffCoef;// 0.05; // diffusion coefficient
    double dThr = m_paramDense.m_paramGotcha.m_fDiffThr;// 0.1;
    int nIter = m_paramDense.m_paramGotcha.m_nDiffIter;// 5;

    int nRow = nSzDiff*2+1;
    int nCol = nSzDiff*2+1;
    double pdDiffMapNext[nRow][nCol];
    double pdDiffMapCurrnet[nRow][nCol];

    for(int j = 0 ; j < nRow; j++){
        for(int i = 0; i < nCol; i++){
            pdDiffMapCurrnet[j][i] = pdDiffMap[j][i];
            pdDiffMapNext[j][i] = 0;
        }
    }

    // define neighbours
    for (int k = 0 ; k < nIter; k++){
        // get diffused
        double pdTemp[nRow][nCol];
        for (int j = 1; j < nRow-1; j++){
            for (int i = 1; i < nCol-1; i++){
                pdDiffMapNext[j][i] = pdDiffMapCurrnet[j][i] + dAlpha *
                                      (pdDiffMapCurrnet[j+1][i] + pdDiffMapCurrnet[j][i+1] +
                                       pdDiffMapCurrnet[j-1][i] + pdDiffMapCurrnet[j][i-1] - 4*pdDiffMapCurrnet[j][i]);
                pdTemp[j][i] = pdDiffMapNext[j][i];
            }
        }
        for(int j = 1 ; j < nRow-1; j++){
            for(int i = 1; i < nCol-1; i++){
                pdDiffMapCurrnet[j][i] = pdTemp[j][i];
            }
        }
    }

    int count = 0;

    for (int j = 1; j < nRow-1; j++){
        for (int i = 1; i < nCol-1; i++){
            if (pdDiffMapNext[j][i] > dThr) { //&& (i != nSzDiff && j != nSzDiff)){ // add as a neighbour
                if (i == nSzDiff && j == nSzDiff) continue;
                CTiePt tpNei;
                tpNei.m_ptL.x = ptLeft.x + i - nSzDiff;
                tpNei.m_ptL.y = ptLeft.y + j - nSzDiff;
                tpNei.m_ptR.x = ptRight.x + i - nSzDiff;
                tpNei.m_ptR.y = ptRight.y + j - nSzDiff;

                vecNeiTp.push_back(tpNei);

                count++;
            }
        }
    }
            // add minimum num of neighbours
            CTiePt tp2;
            tp2.m_ptL = ptLeft + Point2f(0,1);
            tp2.m_ptR = ptRight + Point2f(0,1);
            if (!isHavingTP(vecNeiTp, tp2))
                vecNeiTp.push_back(tp2);

            tp2.m_ptL = ptLeft + Point2f(0,-1);
            tp2.m_ptR = ptRight + Point2f(0,-1);
            if (!isHavingTP(vecNeiTp, tp2))
                vecNeiTp.push_back(tp2);

            //CTiePt tp3;
            tp2.m_ptL = ptLeft + Point2f(1,0);
            tp2.m_ptR = ptRight + Point2f(1,0);
            if (!isHavingTP(vecNeiTp, tp2))
                vecNeiTp.push_back(tp2);

            tp2.m_ptL = ptLeft + Point2f(-1,0);
            tp2.m_ptR = ptRight + Point2f(-1,0);
            if (!isHavingTP(vecNeiTp, tp2))
                vecNeiTp.push_back(tp2);
}

bool CDensify::isHavingTP(vector<CTiePt>& vecNeiTp, CTiePt tp){
    bool bRes = false;
    int nLen = vecNeiTp.size();
    for (int i = 0; i < nLen; i++){
        if (vecNeiTp.at(i) == tp){
            bRes = true;
            break;
        }
    }
    return bRes;
}

void CDensify::removeOutsideImage(vector<CTiePt>& vecNeiTp, const Rect_<float> rectTileL, const Rect_<float> rectImgR){
    vector<CTiePt>::iterator iter;

    for (iter = vecNeiTp.begin(); iter < vecNeiTp.end(); ){

        if (!rectTileL.contains(iter->m_ptL) || !rectImgR.contains(iter->m_ptR))
            vecNeiTp.erase(iter);
       else
            iter++;
    }

}

void CDensify::removePtInLUT(vector<CTiePt>& vecNeiTp, const vector<bool>& pLUT, const int nWidth){
    vector<CTiePt>::iterator iter;

    for (iter = vecNeiTp.begin(); iter < vecNeiTp.end(); ){
        int nX = (int)floor(iter->m_ptL.x);
        int nY = (int)floor(iter->m_ptL.y);
        int nIdx = nY * nWidth + nX;
        if (pLUT[nIdx])
            vecNeiTp.erase(iter);
        else
            iter++;
    }
}

void CDensify::makeTiles(vector< Rect_<float> >& vecRectTiles, int nMin){

    vector< Rect_<float> >::iterator iter;
    for (iter = vecRectTiles.begin(); iter < vecRectTiles.end(); ){
        Rect_<float> rectParent = *iter;
        if (rectParent.width/2 > nMin && rectParent.height/2 > nMin){
            vecRectTiles.erase(iter);
            vector< Rect_<float> > vecChild;
            breakIntoSubRect(rectParent, vecChild);
            vecRectTiles.insert(vecRectTiles.end(), vecChild.begin(),  vecChild.end());
            iter = vecRectTiles.begin();
        }
        else
            iter++;
    }
}

void CDensify::breakIntoSubRect(Rect_<float> rectParent, vector< Rect_<float> >& vecRes){

    float fHalfW = rectParent.width/2.f;
    float fHalfH = rectParent.height/2.f;

    Point2f ptTopLeft[4];
    ptTopLeft[0] = Point2f(rectParent.x, rectParent.y);
    ptTopLeft[1] = Point2f(rectParent.x+fHalfW, rectParent.y);
    ptTopLeft[2] = Point2f(rectParent.x, rectParent.y+fHalfH);
    ptTopLeft[3] = Point2f(rectParent.x+fHalfW, rectParent.y+fHalfH);

    Size sz(fHalfW, fHalfH);
    Rect_<float> rect(ptTopLeft[0], sz);
    vecRes.push_back(rect);

    sz = Size(rectParent.x+rectParent.width - ptTopLeft[1].x, fHalfH);
    rect = Rect(ptTopLeft[1], sz);
    vecRes.push_back(rect);

    sz = Size(fHalfW, rectParent.y+rectParent.height - ptTopLeft[2].y);
    rect = Rect(ptTopLeft[2], sz);
    vecRes.push_back(rect);

    sz = Size(rectParent.x+rectParent.width - ptTopLeft[3].x, rectParent.y+rectParent.height - ptTopLeft[3].y);
    rect = Rect(ptTopLeft[3], sz);
    vecRes.push_back(rect);

}


bool CDensify::doGotcha(const Mat& matImgL, const Mat& matImgR, vector<CTiePt>& vectpSeeds,
                        const CGOTCHAParam& paramGotcha, vector<CTiePt>& vectpAdded){


    bool bRes = true;
    ////////////////////////////////////////////
    // parameter preparation
    cout << "*  MAGiGAN: initialisaing similarity map ...                            *" << endl;
    cout << "*                                                                       *" << endl;
    Mat matSimMap = Mat::ones(matImgL.rows, matImgL.cols, CV_32FC1); // 2D similarity map for diffusion
    matSimMap = matSimMap*-1;

    Size szImgL(matImgL.cols, matImgL.rows);
    cout << "*  MAGiGAN: initialisaing pixel LUT ...                                 *" << endl;
    cout << "*                                                                       *" << endl;

    vector<bool> pLUT;
    for (int i = 0; i< szImgL.area(); i++){
        bool bpLUT = false;
        pLUT.push_back(bpLUT);
    }

    vector< Rect_<float> > vecRectTiles;
    vecRectTiles.push_back(Rect(0., 0., matImgL.cols, matImgL.rows));

    cout << "*  MAGiGAN: making tiles ...                                            *" << endl;
    cout << "*                                                                       *" << endl;
    makeTiles(vecRectTiles, paramGotcha.m_nMinTile);

    if (paramGotcha.m_bNeedInitALSC){

        cout << "*  MAGiGAN: running initial ALSC refinement ...                         *" << endl;
        cout << "*                                                                       *" << endl;

        ALSC alsc(matImgL, matImgR, paramGotcha.m_paramALSC);
        alsc.performALSC(&vectpSeeds);
        vectpSeeds.clear();
        alsc.getRefinedTps(vectpSeeds); // hard-copy
    }

    cout << "*  MAGiGAN: initialise similarity map with seed points.                 *" << endl;
    cout << "*                                                                       *" << endl;

    for (int i = 0; i < (int)vectpSeeds.size(); i++){
        int nX =  (int)floor(vectpSeeds.at(i).m_ptL.x);
        int nY =  (int)floor(vectpSeeds.at(i).m_ptL.y);
        matSimMap.at<float>(nY, nX) = vectpSeeds.at(i).m_fSimVal;

        int nIdx = nY*szImgL.width + nX;
        pLUT[nIdx] = true;
    }

    /////////////////////////
    vectpAdded.clear();
    cout << "*  MAGiGAN: densifying feature correspondences ...                      *" << endl;
    cout << "*                                                                       *" << endl;

    for (int i = 0 ; i < (int)vecRectTiles.size(); i++ ){
          vector<CTiePt> vecRes;
          bRes = bRes && doTileGotcha(matImgL, matImgR, vectpSeeds, paramGotcha, vecRes, vecRectTiles.at(i), matSimMap, pLUT);
          vectpAdded.insert(vectpAdded.end(), vecRes.begin(), vecRes.end());
    }

    return bRes;
}


bool CDensify::doTileGotcha(const Mat& matImgL, const Mat& matImgR, const vector<CTiePt>& vectpSeeds,
                            const CGOTCHAParam& paramGotcha, vector<CTiePt>& vectpAdded,
                            const Rect_<float> rectTileL, Mat& matSimMap, vector<bool>& pLUT){

    vector<CTiePt> vectpSeedTPs; //= vectpSeeds;                // need this hard copy for sorting

    Size szImgL(matImgL.cols, matImgL.rows);
    Rect_<float> rectImgR (0, 0, matImgR.cols, matImgR.rows);
    vectpAdded.clear(); //clear output tp list

    // set indicator buffer
    for (int i = 0 ; i < (int)vectpSeeds.size(); i++){
        CTiePt tp = vectpSeeds.at(i);

        if (rectTileL.contains(tp.m_ptL)){
            vectpSeedTPs.push_back(tp);
        }

    }

    Mat imgL = matImgL;
    Mat imgR = matImgR;
    imgL.convertTo(imgL, CV_8UC1);
    imgR.convertTo(imgR, CV_8UC1);

    unsigned int nGapSize = imgL.cols * imgL.rows;

    //sort(vectpSeedTPs.begin(), vectpSeedTPs.end(), compareTP); // sorted in ascending order

    cout << "*  MAGiGAN: started region growing ...                                  *" << endl;
    cout << "*                                                                       *" << endl;
    cout << "*  MAGiGAN: [--------------------]  0% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
    while (vectpSeedTPs.size() > 0) {
        CTiePt tp = vectpSeedTPs.at(0);
        vectpSeedTPs.erase(vectpSeedTPs.begin());
        vector<CTiePt> vecNeiTp;
        getNeighbour(tp, vecNeiTp, paramGotcha.m_nNeiType, matSimMap);
        removeOutsideImage(vecNeiTp, rectTileL, rectImgR);
        removePtInLUT(vecNeiTp, pLUT, matImgL.cols);

        //ALSC
        if ((int)vecNeiTp.size() > 0){
            float pfData[6] = {0, 0, 0, 0, 0, 0};
            for (int k = 0; k < 4; k++)
                pfData[k] = tp.m_pfAffine[k];
            pfData[4] = tp.m_ptOffset.x;
            pfData[5] = tp.m_ptOffset.y;

            ALSC alsc(matImgL, matImgR, paramGotcha.m_paramALSC);

            alsc.performALSC(&vecNeiTp, (float*) pfData);
            const vector<CTiePt>* pvecRefTPtemp = alsc.getRefinedTps();

            int nLen = pvecRefTPtemp->size();
            if( nLen > 0){
                // append survived neighbours to the seed point list and the seed LUT
                // vector<CRefinedTP>::iterator iterNei;
                for (int i = 0 ; i < nLen; i++){
                    CTiePt tpNei = pvecRefTPtemp->at(i);

                    int nXnei = (int)floor(tpNei.m_ptL.x);
                    int nYnei = (int)floor(tpNei.m_ptL.y);
                    int nIdxNei = nYnei*szImgL.width + nXnei;

                    matSimMap.at<float>(nYnei,nXnei) = tpNei.m_fSimVal;
                    pLUT[nIdxNei] = true;

                    vectpSeedTPs.push_back(tpNei);
                    vectpAdded.push_back(tpNei);
                }
            }
        }
        if (vectpAdded.size()>0){
        if (vectpAdded.size() >= (nGapSize)/20 && vectpAdded.size() < 2*(nGapSize)/20)
            cout << "*  MAGiGAN: [*-------------------]  5% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 2*(nGapSize)/20 && vectpAdded.size() < 3*(nGapSize)/20)
            cout << "*  MAGiGAN: [**------------------] 10% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 3*(nGapSize)/20 && vectpAdded.size() < 4*(nGapSize)/20)
            cout << "*  MAGiGAN: [***-----------------] 15% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 4*(nGapSize)/20 && vectpAdded.size() < 5*(nGapSize)/20)
            cout << "*  MAGiGAN: [****----------------] 20% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 5*(nGapSize)/20 && vectpAdded.size() < 6*(nGapSize)/20)
            cout << "*  MAGiGAN: [*****---------------] 25% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 6*(nGapSize)/20 && vectpAdded.size() < 7*(nGapSize)/20)
            cout << "*  MAGiGAN: [******--------------] 30% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 7*(nGapSize)/20 && vectpAdded.size() < 8*(nGapSize)/20)
            cout << "*  MAGiGAN: [*******-------------] 35% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 8*(nGapSize)/20 && vectpAdded.size() < 9*(nGapSize)/20)
            cout << "*  MAGiGAN: [********------------] 40% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 9*(nGapSize)/20 && vectpAdded.size() < 10*(nGapSize)/20)
            cout << "*  MAGiGAN: [*********-----------] 45% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 10*(nGapSize)/20 && vectpAdded.size() < 11*(nGapSize)/20)
            cout << "*  MAGiGAN: [**********----------] 50% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 11*(nGapSize)/20 && vectpAdded.size() < 12*(nGapSize)/20)
            cout << "*  MAGiGAN: [***********---------] 55% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 12*(nGapSize)/20 && vectpAdded.size() < 13*(nGapSize)/20)
            cout << "*  MAGiGAN: [************--------] 60% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 13*(nGapSize)/20 && vectpAdded.size() < 14*(nGapSize)/20)
            cout << "*  MAGiGAN: [*************-------] 65% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 14*(nGapSize)/20 && vectpAdded.size() < 15*(nGapSize)/20)
            cout << "*  MAGiGAN: [**************------] 70% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 15*(nGapSize)/20 && vectpAdded.size() < 16*(nGapSize)/20)
            cout << "*  MAGiGAN: [***************-----] 75% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 16*(nGapSize)/20 && vectpAdded.size() < 17*(nGapSize)/20)
            cout << "*  MAGiGAN: [****************----] 80% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 17*(nGapSize)/20 && vectpAdded.size() < 18*(nGapSize)/20)
            cout << "*  MAGiGAN: [*****************---] 85% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 18*(nGapSize)/20 && vectpAdded.size() < 19*(nGapSize)/20)
            cout << "*  MAGiGAN: [******************--] 90% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() >= 19*(nGapSize)/20 && vectpAdded.size() < 20*(nGapSize)/20)
            cout << "*  MAGiGAN: [*******************-] 95% OF THE MOTION MAP HAS BEEN DENSIFIED\r" << std::flush;
        else if (vectpAdded.size() == nGapSize)
            cout << "*  MAGiGAN: [********************]100% OF THE MOTION MAP HAS BEEN DENSIFIED";
        }
    }
    cout << "*                                                                       *" << endl;
    return true;
}

bool CDensify::doPGotcha(int nNeiType){

    // pyramid construction
    int nTotLev = getTotPyramidLev(m_paramDense.m_paramGotcha.m_paramALSC.m_nPatch);
    vector<Mat> vecImgPyL;
    vector<Mat> vecImgPyR;
    buildPyramid(m_imgL, vecImgPyL, nTotLev);
    buildPyramid(m_imgR, vecImgPyR, nTotLev);

    vector<CTiePt> vecOrgSeedClone = getIntToFloatSeed(m_vecTPs);
    if (m_paramDense.m_paramGotcha.m_bNeedInitALSC){
        ALSC alsc(m_imgL, m_imgR, m_paramDense.m_paramGotcha.m_paramALSC);
        alsc.performALSC(&vecOrgSeedClone);
        vecOrgSeedClone.clear();
        alsc.getRefinedTps(vecOrgSeedClone); // hard-copy
    }

    CGOTCHAParam paramG = m_paramDense.m_paramGotcha;
    paramG.m_nNeiType = nNeiType;
    paramG.m_bNeedInitALSC = true;

    float fTempLim = m_paramDense.m_paramGotcha.m_paramALSC.m_fEigThr;
    int nTempSz = m_paramDense.m_paramGotcha.m_paramALSC.m_nPatch;
    float fMinRate = 0.3f;
    float fMinRatePatch = 0.5f;

    vector<CTiePt> vecAddedTemp; // addition from previous level
    bool bRes = true;
    for (int i = nTotLev-1; i >= 0; i--){

        // prepare new processing param at level i
        paramG.m_paramALSC.m_nPatch = (int)(nTempSz * (1.f - fMinRatePatch * i/(nTotLev-1)));
        paramG.m_paramALSC.m_fEigThr = fTempLim * (1.f + fMinRate * i/(nTotLev-1));

        // prepare seed for current level
        vector<CTiePt> vecSeedTemp;
        float fDenom = pow(2.f, i);
        int nLen = vecOrgSeedClone.size();
        for (int j = 0; j < nLen; j++){
            CTiePt tp;
            tp = vecOrgSeedClone.at(j);
            tp.m_ptL.x = tp.m_ptL.x / fDenom;
            tp.m_ptL.y = tp.m_ptL.y / fDenom;
            tp.m_ptR.x = tp.m_ptR.x / fDenom;
            tp.m_ptR.y = tp.m_ptR.y / fDenom;
            for (int k = 0; k < 4 ; k++)
                tp.m_pfAffine[k] = 0.;
            tp.m_ptOffset.x = tp.m_ptOffset.x / fDenom;
            tp.m_ptOffset.y = tp.m_ptOffset.y / fDenom;
            vecSeedTemp.push_back(tp);
        }

        // added point propagated points
        nLen = vecAddedTemp.size();
        for (int j = 0; j < nLen; j++){
            vecAddedTemp.at(j).m_ptL.x *= 2.f;
            vecAddedTemp.at(j).m_ptL.y *= 2.f;
            vecAddedTemp.at(j).m_ptR.x *= 2.f;
            vecAddedTemp.at(j).m_ptR.y *= 2.f;
            for (int k = 0; k < 4 ; k++)
                vecAddedTemp.at(j).m_pfAffine[k] = 0.;
            vecAddedTemp.at(j).m_ptOffset.x *= 2.f;
            vecAddedTemp.at(j).m_ptOffset.y *= 2.f;
        }

        if (nLen > 0){
            ALSC alsc(vecImgPyL.at(i), vecImgPyR.at(i), paramG.m_paramALSC);
            alsc.performALSC(&vecAddedTemp);
            vecAddedTemp.clear();
            alsc.getRefinedTps(vecAddedTemp); // hard-copy
        }

        vector<CTiePt> seedIn;
        seedIn.insert(seedIn.end(), vecSeedTemp.begin(), vecSeedTemp.end() );
        seedIn.insert(seedIn.end(), vecAddedTemp.begin(), vecAddedTemp.end());
        vecSeedTemp.clear();

        vector<CTiePt> vecResTemp;
        bRes = bRes && doGotcha(vecImgPyL.at(i), vecImgPyR.at(i), seedIn, paramG, vecResTemp);
        vecAddedTemp.insert(vecAddedTemp.end(), vecResTemp.begin(), vecResTemp.end());

    }

    m_vectpAdded.clear();
    m_vectpAdded.insert(m_vectpAdded.end(), vecOrgSeedClone.begin(), vecOrgSeedClone.end());
    m_vectpAdded.insert(m_vectpAdded.end(), vecAddedTemp.begin(), vecAddedTemp.end());

    return bRes;
}

int CDensify::getTotPyramidLev(int nszPatch) {
    int nRes = 0;

    Size szImgL(m_imgL.cols, m_imgL.rows);
    Size szImgR(m_imgR.cols, m_imgR.rows);
    int nW = 0, nH = 0;

    // get smallest size of width
    nW = min(szImgL.width, szImgR.width);
    nH = min(szImgL.height, szImgR.height);

    int nMinSz = 4*(nszPatch * 2 + 1);
    Size szMin(nMinSz,nMinSz);

    // estimate the minimum of the largest levels
    while(nW > szMin.width || nH > szMin.height){
        nW /= 2;
        nH /= 2;
        nRes++;
    }

    return nRes;
}
