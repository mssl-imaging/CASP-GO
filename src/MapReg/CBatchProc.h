#ifndef CBATCHPROC_H
#define CBATCHPROC_H

#include <iostream>
#include <fstream>
#include <ostream>
#include <vector>

#include <termios.h>
//#include <QtCore/QDir>
#include <QDir>
#include <QString>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <ctime>
#include <string>


#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/photo.hpp"
#include "opencv2/imgcodecs.hpp"

using namespace cv;
using namespace std;

class CBatchProc
{
public://    CBatchProc();
    CBatchProc(string strMeta, string strPrefix);
    ~CBatchProc();

    void doBatchProcessing();


private:
    bool validateProjParam();
    bool validateProjInputs();

    void doCorregistration();

protected:

    string m_strMeta;
    string m_strPrefix;


    string m_strORI;
    string m_strORItmp;
    string m_strORItmptfw;
    string m_strORIRS;
    string m_strORIRStmp;
    string m_strORIRStmptfw;

    string m_strORIReg;

    string m_strRef;
    string m_strRefRS;
    string m_strRefRStmp;
    string m_strRefRStmptfw;

    string m_strDTM;
    string m_strDTMtmp;
    string m_strDTMtmptfw;
    string m_strDTMReg;
    string m_strORITP;
    string m_strRefTP;


    int m_nMinHessian;

private:
    string m_strStartTime;
    string m_strEndTime;
	Mat m_mORI;
	Mat m_mRef;
	
	float m_fORIULx;
	float m_fORIULy;
	float m_fORIRESx;
	float m_fORIRESy;

	float m_fRefULx;
	float m_fRefULy;
	float m_fRefRESx;
	float m_fRefRESy;



};

#endif // CBATCHPROC_H
