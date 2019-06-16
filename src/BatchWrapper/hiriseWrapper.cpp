#include "hiriseWrapper.h"

hiriseWrapper::hiriseWrapper(string strParam)
{
    // introduction
    cout << "*************************************************************************" << endl;
    cout << "*                                                                       *" << endl;
    cout << "*  Welcome to use the CASP-GO auto-DTM system v2.0                      *" << endl;
    cout << "*  >> HiRISE processing                                                 *" << endl;
    cout << "*  Developer: Imaging Group, MSSL, UCL.                                 *" << endl;
    cout << "*                                                                       *" << endl;
    cout << "*  CASP-GO is based on NASA Ames Stereo Pipeline and UCL subroutines    *" << endl;
    cout << "*                                                                       *" << endl;
    cout << "*  Contact email:                                                       *" << endl;
    cout << "*  yu.tao{at}ucl.ac.uk                                                  *" << endl;
    cout << "*  j.muller{at}ucl.ac.uk                                                *" << endl;
    cout << "*                                                                       *" << endl;
    cout << "*  Spicifications for error/warning codes:                              *" << endl;
    cout << "*  (1) input issue;                                                     *" << endl;
    cout << "*  (2) system issue;                                                    *" << endl;
    cout << "*  (3) processing issue.                                                *" << endl;
    cout << "*                                                                       *" << endl;
    cout << "*************************************************************************" << endl << endl;

    cout << "Info: CASP-GO is checking project parameters and inputs." << endl;
    m_strParam = strParam;

    //Prepare all project parameters including input file directories and UCL workflow parameters.
    prepareProjectParam();

    //check all project parameters
    if (!validateProjParam()){
        cerr << "ERROR (2): The project input files cannot be validated" << endl;
        exit(1);
    }

    //Check all project inputs
    if (!validateProjInputs()){
        cerr << "ERROR (2): The project input files not in correct format" << endl;
        exit(1);
    }
    cout << "Info: project parameters/input checking completed." << endl;

}

hiriseWrapper::~hiriseWrapper(){
}

void hiriseWrapper::prepareProjectParam(){
    FileStorage fs(m_strParam, FileStorage::READ);
    FileNode tl = fs["ProjectParam"];

    m_strStereoListFile = (string)tl["strStereoListFile"];
    m_strBaseListFile = (string)tl["strBaseListFile"];
    m_strAspParamFile = (string)tl["strAspParamFile"];
    m_strUclParamFile = (string)tl["strUclParamFile"];

    m_strWorkingDir = (string)tl["strWorkingDir"];
    m_strInputDir = (string)tl["strInputDir"];
    m_strOutputDir = (string)tl["strOutputDir"];

}


bool hiriseWrapper::validateProjParam(){
    bool bRes = true;
    QDir qdir;
    //check stereo list file exists
    if (!qdir.exists(m_strStereoListFile.c_str())){
        cerr << "ERROR (1): Stereo list file does not exist." << endl;
        bRes = false;
    }

    if (!qdir.exists(m_strBaseListFile.c_str())){
        cerr << "ERROR (1): Basemap list file does not exist." << endl;
        bRes = false;
    }

    //check ASP workflow parameter file exists
    if (!qdir.exists(m_strAspParamFile.c_str())){
        cerr << "ERROR (1): Parameter file for ASP workflow does not exist." << endl;
        bRes = false;
    }

    //check UCL workflow parameter file exists
    if (!qdir.exists(m_strUclParamFile.c_str())){
        cerr << "ERROR (1): Parameter file for UCL workflow does not exist." << endl;
        bRes = false;
    }

    //check project working directory (and all executables) exist
    if (!qdir.exists(m_strWorkingDir.c_str())){
        cerr << "ERROR (1): Project working directory does not exist. Please check directory settings in param.xml file or creat the directory manually" << endl;
        bRes = false;
    }

    //check project input directory exist
    if (!qdir.exists(m_strInputDir.c_str())){
        cout << "Warning (1): Project input directory does not exist. Creating..." << endl;
        qdir.mkpath(QString(m_strInputDir.c_str()));
    }
    //check project output directory exist
    if (!qdir.exists(m_strOutputDir.c_str())){
        cout << "Warning (1): Project output directory does not exist. Creating..." << endl;
        qdir.mkpath(QString(m_strOutputDir.c_str()));
    }

    return bRes;
}


bool hiriseWrapper::validateProjInputs(){
    bool bRes = true;
    // Read in stereo image list file

    int nNum = 0;
    string buf;
    ifstream sfStereoList(m_strStereoListFile.c_str());
    cout << "Info: CASP-GO is loading stereo image DIRs from the stereo list file provided." << endl;
    while(sfStereoList)
    {
        if( getline( sfStereoList, buf ) )
        {
            nNum ++;
            m_strvStereoList.push_back(buf);
        }
    }
    sfStereoList.close();
    cout << "Info: a total number of " << nNum << " stereo image DIRs have been loaded to buf." << endl;
    if(nNum%2 != 0){
        cerr << "ERROR (1): Total image numbers from the stereo list file is not even. Please check the stereo list file for odd entries." << endl;
        bRes = false;
        exit(1);
    }

    for (int i=0; i<nNum; i+=2){
        string strL = m_strvStereoList[i];
        string strR = m_strvStereoList[i+1];
        const size_t last_slash_idxL = strL.find_last_of("\\/");
        const size_t last_slash_idxR = strR.find_last_of("\\/");
        if (std::string::npos != last_slash_idxL)
        {
            strL.erase(0, last_slash_idxL + 1);
        }
        if (std::string::npos != last_slash_idxR)
        {
            strR.erase(0, last_slash_idxR + 1);
        }
        const size_t period_idxL = strL.rfind('.');
        if (std::string::npos != period_idxL)
        {
            strL.erase(period_idxL);
        }
        const size_t period_idxR = strR.rfind('.');
        if (std::string::npos != period_idxR)
        {
            strR.erase(period_idxR);
        }
        m_strvLeftIDList.push_back(strL);
        m_strvRightIDList.push_back(strR);
    }

    int nNumL = m_strvLeftIDList.size();
    int nNumR = m_strvRightIDList.size();
    cout << "Info: a total number of " << nNumL << " left image IDs have been loaded to buf." << endl;
    cout << "Info: a total number of " << nNumR << " right image IDs have been loaded to buf." << endl;

    nNum = 0;
    ifstream sfBaseList(m_strBaseListFile.c_str());
    cout << "Info: CASP-GO is loading basemap directories from the basemap list file provided." << endl;
    while(sfBaseList)
    {
        if( getline( sfBaseList, buf ) )
        {
            nNum ++;
            m_strvBaseList.push_back(buf);
        }
    }
    sfBaseList.close();
    cout << "Info: a total number of " << nNum << " basemap directories have been loaded to buf." << endl;


    if(m_strvLeftIDList.size()!= m_strvRightIDList.size()){
        cerr << "ERROR (1): Total image numbers from the left ID list and right ID list files are different." << endl;
        bRes = false;
        exit(1);
    }

    if(m_strvLeftIDList.size()!= m_strvBaseList.size()){
        cerr << "ERROR (1): Total number of the stereo pairs does not equal to the total number of lines of the basemap entries." << endl;
        bRes = false;
        exit(1);
    }

    FileStorage fs(m_strUclParamFile, FileStorage::READ);
    FileNode tl = fs["processParam"];

    m_nThreads = (int)tl["nThreads"];

    return bRes;
}



void hiriseWrapper::doBatchProcessing(){
    int n = m_strvStereoList.size();
    int nCount = 1;
    for (nCount = 1; nCount <= n/2; nCount++){
        cout << endl;
        cout << endl;
        cout << "Info: CASP-GO is initialising number " << nCount << " of the " << n/2 << " stereo pairs." << endl;
        //dataDownload(nCount);

        if (checkDataDownload(nCount)){
            cout << endl;
            cout << endl;
            cout << "Info: CASP-GO has checked the stereo pair just loaded. They look OK." << endl;

            char buff[20];
            time_t now = time(NULL);
            strftime(buff, 20, "%Y-%m-%dT%H:%M", localtime(&now));
            m_strStartTime.clear();
            m_strStartTime =  buff;
            cout << endl;
            cout << endl;
            cout << "Info: CASP-GO started [ " << m_strStartTime << " ]." << endl;

            cout << "Info: CASP-GO is pre-processing number " << nCount << " of the " << n/2 << " stereo pairs." << endl;
            dataConversion(nCount);

            if (checkDataConversion(nCount)){
                cout << endl;
                cout << endl;
                cout << "Info: CASP-GO has checked the stereo pair just pre-processed. They look OK." << endl;
                cout << "Info: CASP-GO is processing number " << nCount << " of the " << n/2 << " stereo pairs (Stage-1)." << endl;
                dataProcessP1(nCount);

                if (checkDataProcessP1(nCount)){
                    cout << endl;
                    cout << endl;
                    cout << "Info: CASP-GO has checked the Stage-1 processing result. They look OK." << endl;
                    cout << "Info: CASP-GO is processing number " << nCount << " of the " << n/2 << " stereo pairs (Stage-2)." << endl;
                    dataProcessP2(nCount);

                    if (checkDataProcessP2(nCount)){
                        cout << endl;
                        cout << endl;
                        cout << "Info: CASP-GO has checked the Stage-2 processing result. They look OK." << endl;
                        cout << "Info: CASP-GO is processing number " << nCount << " of the " << n/2 << " stereo pairs (Stage-3)." << endl;
                        dataProcessP3(nCount);

                        if (checkDataProcessP3(nCount)){
                            cout << endl;
                            cout << endl;
                            cout << "Info: CASP-GO has checked the Stage-3 processing result. They look OK." << endl;
                            cout << "Info: CASP-GO is processing number " << nCount << " of the " << n/2 << " stereo pairs (Stage-4)." << endl;
                            dataProcessP4(nCount);

                            char buff[20];
                            time_t now = time(NULL);
                            strftime(buff, 20, "%Y-%m-%dT%H:%M", localtime(&now));
                            m_strEndTime.clear();
                            m_strEndTime = buff;
                            cout << "Info: CASP-GO finished [ " << m_strEndTime << " ]." << endl;
                            cout << endl;
                            cout << endl;
                            cout << "Info: CASP-GO is saving metadata file for number " << nCount << " of the " << n/2 << " stereo pairs." << endl;
                            saveMetaData(nCount);
                            cout << endl;
                            cout << endl;
                            cout << "Info: CASP-GO has completed processing number " << nCount << " of the " << n/2 << " stereo pairs." << endl;
                        }
                    }
                }
            }
        }
    }

    cout << "******************" << endl;
    cout << "*                *" << endl;
    cout << "*  All done >_<  *" << endl;
    cout << "*                *" << endl;
    cout << "******************" << endl;
}

void hiriseWrapper::dataDownload(int nNum){
    int m_nNum = nNum;
    int m_nLeft = 2*m_nNum - 2;
    int m_nRight = 2*m_nNum - 1;
    string m_strLeftImageURL = m_strvStereoList[m_nLeft];
    string m_strRightImageURL = m_strvStereoList[m_nRight];

    ostringstream strCmdLeft;
    strCmdLeft << "wget -P " << m_strInputDir << " " << m_strLeftImageURL;
    system(strCmdLeft.str().c_str());

    ostringstream strCmdRight;
    strCmdRight << "wget -P " << m_strInputDir << " " << m_strRightImageURL;
    system(strCmdRight.str().c_str());
}

bool hiriseWrapper::checkDataDownload(int nNum){
    bool bRes = true;

    cout << "Info: CASP-GO is checking the stereo pairs that the user has downloaded." << endl;

    /*
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strLeftImageFile = m_strInputDir + m_strLeftImageID + ".IMG";
    QDir qdir;
    if (!qdir.exists(m_strLeftImageFile.c_str())){
        cerr << "ERROR (2): the download of number " << m_nNum+1 << " of stereo left image (ID: "
             << m_strLeftImageID << ") has failed. Skipping this stereo pair."
             << " Please check from http://pds-imaging.jpl.nasa.gov for more information." << endl;
        bRes = false;
    }

    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strRightImageFile = m_strInputDir + m_strRightImageID + ".IMG";

    if (!qdir.exists(m_strRightImageFile.c_str())){
        cerr << "ERROR (2): the download of number " << m_nNum+1 << " of stereo right image (ID: "
             << m_strRightImageID << ") has failed. Skipping this stereo pair."
             << " Please check from http://pds-imaging.jpl.nasa.gov for more information." << endl;
        bRes = false;
    }
    */
    return bRes;
}

void hiriseWrapper::dataConversion(int nNum){
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;
    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strLeftImageFile = m_strvStereoList[m_nNum*2] + "*.IMG";
    string m_strLeftImageFileConverted1 = m_strInputDir + m_strLeftImageID + ".mos_hijitreged.norm.cub";

    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strRightImageFile = m_strvStereoList[m_nNum*2+1] + "*.IMG";
    string m_strRightImageFileConverted1 = m_strInputDir + m_strRightImageID + ".mos_hijitreged.norm.cub";


    /*
    cout << "Info: CASP-GO is preparing a list file for the stereo pair just requested." << endl;
    m_nNum = m_nNum + 1;

    stringstream ssNum;
    ssNum << m_nNum;
    string m_strNum = ssNum.str();
    string m_strStereoProcessingList = m_strInputDir + "stereo-" + m_strNum + ".txt";

    ofstream sfStereoProcessingList;
    sfStereoProcessingList.open(m_strStereoProcessingList.c_str());
    if (sfStereoProcessingList.is_open()){
        sfStereoProcessingList << m_strLeftImageFile << endl;
        sfStereoProcessingList << m_strRightImageFile << endl;
        sfStereoProcessingList.close();
    }
    */

    cout << "Info: CASP-GO is converting the stereo pair from PDS to USGS-ISIS format which is required by ASP." << endl;

    string m_strConversionSW = m_strWorkingDir + "ASP/bin/hiedr2mosaic.py";
    ostringstream strCmdConvert1;
    strCmdConvert1 << m_strConversionSW << " " << m_strLeftImageFile;
    system(strCmdConvert1.str().c_str());

    ostringstream strCmdConvert2;
    strCmdConvert2 << m_strConversionSW << " " << m_strRightImageFile;
    system(strCmdConvert2.str().c_str());

    ostringstream strCmdMove1;
    strCmdMove1 << "mv ./" << m_strLeftImageID << ".mos_hijitreged.norm.cub" << " " << m_strInputDir;
    system(strCmdMove1.str().c_str());
    ostringstream strCmdMove2;
    strCmdMove2 << "mv ./" << m_strRightImageID << ".mos_hijitreged.norm.cub" << " " << m_strInputDir;
    system(strCmdMove2.str().c_str());

    string m_strConversionSW2 = m_strWorkingDir + "ASP/bin/cam2map4stereo.py";
    ostringstream strCmdConvert3;
    strCmdConvert3 << m_strConversionSW2 << " " << m_strLeftImageFileConverted1 << " " << m_strRightImageFileConverted1;
    system(strCmdConvert3.str().c_str());


}

bool hiriseWrapper::checkDataConversion(int nNum){
    bool bRes = true;
    cout << "Info: CASP-GO is checking the stereo pair just pre-processed." << endl;
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strLeftImageFileConverted2 = m_strInputDir + m_strLeftImageID + ".map.cub";
    QDir qdir;
    if (!qdir.exists(m_strLeftImageFileConverted2.c_str())){
        cerr << "ERROR (3): the pre-process of number " << m_nNum+1 << " of stereo left image (ID: "
             << m_strLeftImageID << ") has failed. Skipping this stereo pair."
             << " Please check the data source using qview manually,"
             << "Or if this is a new image, maybe the current spice kernel needs to be updated to pre-process this pair."
             << endl;
        bRes = false;
    }

    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strRightImageFileConverted2 = m_strInputDir + m_strRightImageID + ".map.cub";

    if (!qdir.exists(m_strRightImageFileConverted2.c_str())){
        cerr << "ERROR (3): the pre-process of number " << m_nNum+1 << " of stereo right image (ID: "
             << m_strRightImageID << ") has failed. Skipping this stereo pair."
             << " Please check the data source using qview manually,"
             << " Or if this is a new image, maybe the current spice kernel needs to be updated to pre-process this pair."
             << endl;
        bRes = false;
    }

    return bRes;
}

void hiriseWrapper::dataProcessP1(int nNum){
    cout << "Info: CASP-GO is running Bundle Adjustment." << endl;
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strP1SW = m_strWorkingDir + "ASP/bin/bundle_adjust";

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strLeftImageFile = m_strInputDir + m_strLeftImageID + ".map.cub";
    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strRightImageFile = m_strInputDir + m_strRightImageID + ".map.cub";

    string m_strResID = m_strLeftImageID + "-" + m_strRightImageID;
    string m_strResBA = m_strOutputDir + m_strResID + "/ba";

    ostringstream strCmdP1;
    strCmdP1 << m_strP1SW << " " << m_strLeftImageFile << " " << m_strRightImageFile << " -o " << m_strResBA;
    system(strCmdP1.str().c_str());
}

bool hiriseWrapper::checkDataProcessP1(int nNum){
    bool bRes = true;
    cout << "Info: CASP-GO is checking the stage-1 processing result." << endl;
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strResID = m_strLeftImageID + "-" + m_strRightImageID;

    string m_strLeftImageFile = m_strOutputDir + m_strResID + "/ba-" + m_strLeftImageID + ".map.adjust";
    string m_strRightImageFile = m_strOutputDir + m_strResID + "/ba-" + m_strRightImageID + ".map.adjust";

    QDir qdir;
    if (!qdir.exists(m_strLeftImageFile.c_str())){
        cerr << "ERROR (3): the process (Stage-1) of number " << m_nNum+1 << " of stereo left image (ID: "
             << m_strLeftImageID << ") has failed. Skipping this stereo pair."
             << " Please check if the SPICE data has been successfully attached to the converted cub file." << endl;
        bRes = false;
    }

    if (!qdir.exists(m_strRightImageFile.c_str())){
        cerr << "ERROR (3): the process (Stage-1) of number " << m_nNum+1 << " of stereo right image (ID: "
             << m_strRightImageID << ") has failed. Skipping this stereo pair."
             << " Please check if the SPICE data has been successfully attached to the converted cub file." << endl;
        bRes = false;
    }

    return bRes;
}

void hiriseWrapper::dataProcessP2(int nNum){
    cout << "Info: CASP-GO is running stereo processing." << endl;
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strP2SW = m_strWorkingDir + "ASP/bin/stereo_pprc";

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strLeftImageFile = m_strInputDir + m_strLeftImageID + ".map.cub";
    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strRightImageFile = m_strInputDir + m_strRightImageID + ".map.cub";

    string m_strResID = m_strLeftImageID + "-" + m_strRightImageID;
    string m_strResP2 = m_strOutputDir + m_strResID + "/" + m_strResID;
    string m_strResBA = m_strOutputDir + m_strResID + "/ba";

    ostringstream strCmdP21;
    strCmdP21 << m_strP2SW << " -s " << m_strAspParamFile << " "
             << m_strLeftImageFile << " " << m_strRightImageFile << " "
             << m_strResP2 << " --bundle-adjust-prefix " << m_strResBA
             << " --threads " << m_nThreads;
    system(strCmdP21.str().c_str());

    ostringstream strCmdP22;
    m_strP2SW = m_strWorkingDir + "ASP/bin/stereo_corr";
    strCmdP22 << m_strP2SW << " -s " << m_strAspParamFile << " "
             << m_strLeftImageFile << " " << m_strRightImageFile << " "
             << m_strResP2 << " --bundle-adjust-prefix " << m_strResBA
             << " --threads " << m_nThreads << " --corr-timeout 600";
    system(strCmdP22.str().c_str());

    ostringstream strCmdP23;
    m_strP2SW = m_strWorkingDir + "ASP/bin/stereo_rfne";
    strCmdP23 << m_strP2SW << " -s " << m_strAspParamFile << " "
             << m_strLeftImageFile << " " << m_strRightImageFile << " "
             << m_strResP2 << " --bundle-adjust-prefix " << m_strResBA
             << " --threads " << m_nThreads << " --corr-timeout 600";
    system(strCmdP23.str().c_str());

    ostringstream strCmdP24;
    m_strP2SW = m_strWorkingDir + "ASP/bin/stereo_fltr";
    strCmdP24 << m_strP2SW << " -s " << m_strAspParamFile << " "
             << m_strLeftImageFile << " " << m_strRightImageFile << " "
             << m_strResP2 << " --bundle-adjust-prefix " << m_strResBA
             << " --threads " << m_nThreads << " --corr-timeout 600";
    system(strCmdP24.str().c_str());
}

bool hiriseWrapper::checkDataProcessP2(int nNum){
    bool bRes = true;
    cout << "Info: CASP-GO is checking the stage-2 processing result." << endl;
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strResID = m_strLeftImageID + "-" + m_strRightImageID;
    string m_strResP2 = m_strOutputDir + m_strResID + "/" + m_strResID + "-F.tif";

    QDir qdir;
    if (!qdir.exists(m_strResP2.c_str())){
        cerr << "ERROR (3): the process (Stage-2) of number " << m_nNum+1 << " of stereo pair (ID: "
             << m_strResID << ") has failed. Skipping this stereo pair."
             << " Please check if the input images are in very bad quality." << endl;
        bRes = false;
    }

    return bRes;

}

void hiriseWrapper::dataProcessP3(int nNum){
    cout << "Info: CASP-GO is performing Gotcha refinement." << endl;
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;


    string m_strP3SW = m_strWorkingDir + "UCL/disp_denoise";

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strLeftImageFile = m_strInputDir + m_strLeftImageID + ".map.cub";
    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strRightImageFile = m_strInputDir + m_strRightImageID + ".map.cub";

    string m_strResID = m_strLeftImageID + "-" + m_strRightImageID;
    string m_strResP2 = m_strOutputDir + m_strResID + "/" + m_strResID + "-F.tif";
    string m_strResP2old = m_strOutputDir + m_strResID + "/" + m_strResID + "-F-old.tif";
    string m_strResP2PC = m_strOutputDir + m_strResID + "/" + m_strResID + "-PC.tif";
    string m_strResP2L = m_strOutputDir + m_strResID + "/" + m_strResID + "-L.tif";

    string m_strResP3 = m_strOutputDir + m_strResID + "/" + m_strResID;
    string m_strResP3c1 = m_strOutputDir + m_strResID + "/" + m_strResID + "-c1.tif";
    string m_strResP3c2 = m_strOutputDir + m_strResID + "/" + m_strResID + "-c2.tif";
    string m_strResP3c3 = m_strOutputDir + m_strResID + "/" + m_strResID + "-c3.tif";

    string m_strResP3c1denoised = m_strOutputDir + m_strResID + "/" + m_strResID + "-c1_denoised.tif";
    string m_strResP3c2denoised = m_strOutputDir + m_strResID + "/" + m_strResID + "-c2_denoised.tif";
    string m_strResP3c3denoised = m_strOutputDir + m_strResID + "/" + m_strResID + "-c3_denoised.tif";

    string m_strResBA = m_strOutputDir + m_strResID + "/ba";

    ostringstream strCmdP31;
    strCmdP31 << "gdal_translate " << m_strResP2 << " -of GTiff -b 1 " << m_strResP3c1;
    system(strCmdP31.str().c_str());

    ostringstream strCmdP32;
    strCmdP32 << "gdal_translate " << m_strResP2 << " -of GTiff -b 2 " << m_strResP3c2;
    system(strCmdP32.str().c_str());

    ostringstream strCmdP34;
    strCmdP34 << "gdal_translate " << m_strResP2 << " -of GTiff -b 3 " << m_strResP3c3;
    system(strCmdP34.str().c_str());

    ostringstream strCmdP35;
    strCmdP35 << m_strP3SW << " "
             << m_strResP3c1 << " " << m_strResP3c1denoised << " "
             << m_strResP3c2 << " " << m_strResP3c2denoised << " "
             << m_strResP3c3 << " " << m_strResP3c3denoised;
    system(strCmdP35.str().c_str());


    //if(checkSize){
    //    ostringstream strCmdP36;
    //    strCmdP36 << "mv " << m_strResP2 << " " << m_strResP2old;
    //    system(strCmdP36.str().c_str());

    //    ostringstream strCmdP37;
    //    strCmdP37 << "gdal_merge.py -separate " << m_strResP3c1denoised << " " << m_strResP3c2denoised << " " << m_strResP3c3denoised
    //             << " -o " << m_strResP2 << " -of GTiff";
    //    system(strCmdP37.str().c_str());
    //}
    ostringstream strCmdP38;
    m_strP3SW = m_strWorkingDir + "ASP/bin/stereo_tri";
    strCmdP38 << m_strP3SW << " -s " << m_strAspParamFile << " "
             << m_strLeftImageFile << " " << m_strRightImageFile << " "
             << m_strResP3 << " --bundle-adjust-prefix " << m_strResBA
             << " --threads " << m_nThreads;
    system(strCmdP38.str().c_str());

    ostringstream strCmdP39;
    m_strP3SW = m_strWorkingDir + "ASP/bin/point2dem";
    strCmdP39 << m_strP3SW << " " << m_strResP2PC << " --orthoimage " << m_strResP2L
              << " -r Mars --dem-hole-fill-len 1250 --orthoimage-hole-fill-len 1250 --median-filter-params 25 25.5"
              << " --threads " << m_nThreads;
    system(strCmdP39.str().c_str());

}

bool hiriseWrapper::checkDataProcessP3(int nNum){
    bool bRes = true;
    cout << "Info: CASP-GO is checking the stage-3 processing result." << endl;

    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strResID = m_strLeftImageID + "-" + m_strRightImageID;
    string m_strResP3DEM = m_strOutputDir + m_strResID + "/" + m_strResID + "-DEM.tif";
    string m_strResP3DRG = m_strOutputDir + m_strResID + "/" + m_strResID + "-DRG.tif";

    QDir qdir;
    if (!qdir.exists(m_strResP3DEM.c_str())){
        cerr << "ERROR (3): the process (Stage-3) of number " << m_nNum+1 << " of stereo pair (ID: "
             << m_strResID << ") has failed. Skipping this stereo pair."
             << " No DTM file found. This might be a processing problem, please revise the stereo pair manually." << endl;
        bRes = false;
    }

    if (!qdir.exists(m_strResP3DRG.c_str())){
        cerr << "ERROR (3): the process (Stage-3) of number " << m_nNum+1 << " of stereo pair (ID: "
             << m_strResID << ") has failed. Skipping this stereo pair."
             << " No ORI file found. This might be a processing problem, please revise the stereo pair manually." << endl;
        bRes = false;
    }

    return bRes;
}

void hiriseWrapper::dataProcessP4(int nNum){
    cout << "Info: CASP-GO is correcting the projection information of DTM and ORI file." << endl;
    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strResID = m_strLeftImageID + "-" + m_strRightImageID;
    string m_strResP4 = m_strOutputDir + m_strResID + "/" + m_strResID;
    string m_strResP3DEM = m_strOutputDir + m_strResID + "/" + m_strResID + "-DEM.tif";
    string m_strResP3DRG = m_strOutputDir + m_strResID + "/" + m_strResID + "-DRG.tif";
    string m_strResP4PROJ = m_strOutputDir + m_strResID + "/" + m_strResID + "-PROJ.tif";
    string m_strResP4DTM = m_strOutputDir + m_strResID + "/" + m_strResID + "-DTM.tif";
    string m_strResP4ORI = m_strOutputDir + m_strResID + "/" + m_strResID + "-ORI.tif";
    string m_strResP4Base = m_strOutputDir + m_strResID + "/" + m_strResID + "-Base.tif";

    ostringstream strCmdP41;
    strCmdP41 << "gdalwarp -t_srs '+proj=eqc +lat_ts=0 +lat_0=0 +lon_0=0 +x_0=0 +y_0=0 "
             << "+a=3396000 +b=3396000 +units=m +no_defs' -r cubic "
             << "-srcnodata -3.4028234663852886e+38 -dstnodata -3.4028234663852886e+38 -tr 0.25 0.25 "
             << m_strResP3DEM << " " << m_strResP4PROJ;
    system(strCmdP41.str().c_str());

    ostringstream strCmdP42;
    strCmdP42 << "gdalwarp -t_srs '+proj=eqc +lat_ts=0 +lat_0=0 +lon_0=0 +x_0=0 +y_0=0 "
             << "+a=3396000 +b=3396000 +units=m +no_defs' -r cubic "
             << "-srcnodata -3.4028234663852886e+38 -dstnodata -3.4028234663852886e+38 -tr 1 1 "
             << m_strResP4PROJ << " " << m_strResP4DTM;
    system(strCmdP42.str().c_str());

    ostringstream strCmdP43;
    strCmdP43 << "gdalwarp -t_srs '+proj=eqc +lat_ts=0 +lat_0=0 +lon_0=0 +x_0=0 +y_0=0 "
             << "+a=3396000 +b=3396000 +units=m +no_defs' -r cubic "
             << "-srcnodata -3.4028234663852886e+38 -dstnodata -3.4028234663852886e+38 -tr 0.25 0.25 "
             << m_strResP3DRG << " " << m_strResP4ORI;
    system(strCmdP43.str().c_str());


    string m_strBase = m_strvBaseList[m_nNum];
    string m_strNoBase = "N/A";
    if (m_strBase != m_strNoBase){
        ostringstream strCmdP44;
        strCmdP44 << "cp " << m_strBase << " " << m_strResP4Base;
        system(strCmdP44.str().c_str());

        cout << "Info: CASP-GO is co-registering the DTM and ORI to the basemap file provided." << endl;
        //run co-registration.
        ostringstream strCmdP441;
        string m_strP4SW = m_strWorkingDir + "UCL/RegisterMap";
        strCmdP441 << m_strP4SW << " " << m_strUclParamFile << " " << m_strResP4;
        system(strCmdP441.str().c_str());
        cout << "Info: co-registration completed." << endl;

    }

    else
        cout << "Info: No basemap file provided, skipping co-registration." << endl;
}

void hiriseWrapper::saveMetaData(int nNum){
    cout << "Info: CASP-GO is saving a Metadata file." << endl;

    FileStorage fs(m_strUclParamFile, FileStorage::READ);
    FileNode tl = fs["sGotchaParam"];
    int m_nALSCIteration = (int)tl["nALSCIteration"];
    float m_fMaxEigenValue = (float)tl["fMaxEigenValue"];
    int m_nALSCKernel = (int)tl["nALSCKernel"];
    int m_nGrowNeighbour = (int)tl["nGrowNeighbour"];

    tl = fs["MLParam"];
    int m_nMLKernel = (int)tl["nMLKernel"];
    int m_nMLIter = (int)tl["nMLIter"];

    tl = fs["ORSParam"];
    float m_fMaxDiff = (float)tl["fMaxDiff"];
    int m_nPercentDiff = (int)tl["nPercentDiff"];
    int m_nDiffKernel = (int)tl["nDiffKernel"];
    float m_fPatchThreshold = (float)tl["fPatchThreshold"];
    int m_nPercentReject = (int)tl["nPercentReject"];
    int m_nErode = (int)tl["nErode"];

    tl = fs["coKrigingParam"];
    int m_nNeighbourLimit = (int)tl["nNeighbourLimit"];
    int m_nDistLimit = (int)tl["nDistLimit"];
    float m_fSpatialResRatio = (float)tl["fSpatialResRatio"];

    tl = fs["MSASURFParam"];
    int m_nOctave = (int)tl["nOctave"];
    int m_nEdgeThreshold = (int)tl["nEdgeThreshold"];
    float m_fMatchCoeff = (float)tl["fMatchCoeff"];
    int m_nLayer = (int)tl["nLayer"];

    int m_nNum = nNum;
    m_nNum = m_nNum - 1;

    string m_strLeftImageID = m_strvLeftIDList[m_nNum];
    string m_strRightImageID = m_strvRightIDList[m_nNum];
    string m_strResID = m_strLeftImageID + "-" + m_strRightImageID;
    string m_strMeta = m_strOutputDir + m_strResID + "/" + m_strResID + "-Meta.txt";

    ofstream sfMeta;
    sfMeta.open(m_strMeta.c_str(), ios::app | ios::out);

    if (sfMeta.is_open()){
        sfMeta << "Object = AutoDTM" << endl;

        sfMeta << "  Object = ProductInfo" << endl;
        sfMeta << "    Object = Processing" << endl;
        sfMeta << "      SoftwareName = CASP-GO" << endl;
        sfMeta << "      SoftwareVersion = 1.2" << endl;
        sfMeta << "      OperatingSystem = \"RHEL v7.2\"" << endl;
        sfMeta << "      ProcessingStartTime = " << m_strStartTime << endl;
        sfMeta << "      ProcessingEndTime = " << m_strEndTime << endl;
        sfMeta << "      ProcessingOrganisation = TBD" << endl;
        sfMeta << "      ProcessingResource = \"TBD\"" << endl;
        sfMeta << "      ContactPerson = \"Yu Tao\"" << endl;
        sfMeta << "      ContactEmail = \"yu.tao{at}ucl.ac.uk\"" << endl;
        sfMeta << "    End_Object" << endl;
        sfMeta << endl;

        sfMeta << "    Object = Data" << endl;
        sfMeta << "      ID = " << m_strResID << endl;
        sfMeta << "      Format = GeoTiff" << endl;
        sfMeta << "      Band = 1" << endl;
        sfMeta << "      BitDepth = 32f" << endl;
        sfMeta << "      DTMResolution = 1" << endl;
        sfMeta << "      ORIResolution = 0.25" << endl;
        sfMeta << "      Unit = Metre" << endl;
        sfMeta << "      NodataValue = -3.4028234663852886e+38" << endl;
        sfMeta << "      Projection = Equirectangular" << endl;
        sfMeta << "    End_Object" << endl;
        sfMeta << "  End_Object" << endl;
        sfMeta << endl;

        sfMeta << "  Object = Algorithm" << endl;
        sfMeta << "    Group = ASP" << endl;
        sfMeta << "      Name = \"Ames Stereo Pipeline Function Parameters\"" << endl;
        sfMeta << "      InitialCorrKernel = N/A" << endl;
        sfMeta << "      RefinementCorrKernel = N/A" << endl;
        sfMeta << "      RefinementIteration = N/A" << endl;
        sfMeta << "    End_Group" << endl;
        sfMeta << endl;

        sfMeta << "    Group = sGotcha" << endl;
        sfMeta << "      Name = \"Adaptive Least Squares Correlation and Region growing Parameters\"" << endl;
        sfMeta << "      ALSCIteration = " << m_nALSCIteration << endl;
        sfMeta << "      MaxEigenValue = " << m_fMaxEigenValue << endl;
        sfMeta << "      ALSCKernel = " << m_nALSCKernel << endl;
        sfMeta << "      GrowNeighbour = " << m_nGrowNeighbour << endl;
        sfMeta << "    End_Group" << endl;
        sfMeta << endl;

        sfMeta << "    Group = ML" << endl;
        sfMeta << "      Name = \"Fast Maximum Likelihood Matching Parameters\"" << endl;
        sfMeta << "      MLKernel = " << m_nMLKernel << endl;
        sfMeta << "      MLIter = " << m_nMLIter << endl;
        sfMeta << "    End_Group" << endl;
        sfMeta << endl;

        sfMeta << "    Group = ORS" << endl;
        sfMeta << "      Name = \"Outlier Rejection Schemes Parameters\"" << endl;
        sfMeta << "      MaxDiff = " << m_fMaxDiff << endl;
        sfMeta << "      PercentDiff = " << m_nPercentDiff << endl;
        sfMeta << "      DiffKernel = " << m_nDiffKernel << endl;
        sfMeta << "      PatchThreshold = " << m_fPatchThreshold << endl;
        sfMeta << "      PercentReject = " << m_nPercentReject << endl;
        sfMeta << "      Erode = " << m_nErode << endl;
        sfMeta << "    End_Group" << endl;
        sfMeta << endl;

        sfMeta << "    Group = coKriging" << endl;
        sfMeta << "      Name = \"Co-Kriging Interpolation Parameters\"" << endl;
        sfMeta << "      NeighbourLimit = " << m_nNeighbourLimit << endl;
        sfMeta << "      DistLimit = " << m_nDistLimit << endl;
        sfMeta << "      SpatialResRatio = " << m_fSpatialResRatio << endl;
        sfMeta << "    End_Group" << endl;
        sfMeta << endl;

        sfMeta << "    Group = MSA-SIFT" << endl;
        sfMeta << "      Name = \"Mutual Shape Adapted Scale Invariant Feature Transform Co-registion Parameters\"" << endl;
        sfMeta << "      nOctave = " << m_nOctave << endl;
        sfMeta << "      EdgeThreshold = " << m_nEdgeThreshold << endl;
        sfMeta << "      MatchCoeff = " << m_fMatchCoeff << endl;
        sfMeta << "      nLayer = " << m_nLayer << endl;
        sfMeta << "    End_Group" << endl;
        sfMeta << "  End_Object" << endl;
        sfMeta << "End_Object" << endl;
        sfMeta << "End" << endl;

        sfMeta.close();
    }
    else
        cout << "ERROR (2): Unable to save Metadata file for number " << m_nNum+1 << " of stereo pair (ID: "
             << m_strResID << "). Skipping this stereo pair."
             << " Please check if the project output directory is full." << endl;

}
