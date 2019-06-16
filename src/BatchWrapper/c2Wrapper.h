#ifndef C2WRAPPER_H
#define C2WRAPPER_H

#include <iostream>
#include <fstream>
#include <ostream>
#include <vector>

#include <termios.h>
#include <QDir>
#include <QString>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <ctime>
#include <string>

#include <opencv2/opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/features2d.hpp>
#include "opencv2/photo.hpp"

using namespace cv;
using namespace std;
using std::string;

class c2Wrapper
{
public://    c2Wrapper();
    c2Wrapper(string strParam);
    ~c2Wrapper();

    void prepareProjectParam(); // this include input/output file directories
    void doBatchProcessing();


private:
    bool validateProjParam();
    bool validateProjInputs();

    void dataPrep(int nNum);
    bool checkDataPrep(int nNum);
    void dataConversion(int nNum);
    bool checkDataConversion(int nNum);
    void dataProcessP1(int nNum);
    bool checkDataProcessP1(int nNum);
    void dataProcessP2(int nNum);
    bool checkDataProcessP2(int nNum);
    void dataProcessP3(int nNum);
    bool checkDataProcessP3(int nNum);
    void dataProcessP4(int nNum);

    void saveMetaData(int nNum);

    void initialFeatureMatching(int nNum);
    void densification(int nNum);

protected:
    string m_strParam;

    int m_nMode;

    string m_strStereoListFile;
    string m_strRPCListFile;
    string m_strBaseListFile;
    string m_strAspParamFile;
    string m_strUclParamFile;

    string m_strWorkingDir;
    string m_strInputDir;
    string m_strOutputDir;

    vector<string> m_strvStereoList;
    vector<string> m_strvRPCList;
    vector<string> m_strvLeftIDList;
    vector<string> m_strvRightIDList;
    vector<string> m_strvBaseList;

private:
    string m_strStartTime;
    string m_strEndTime;
    int m_nThreads;

};

#endif
