#include "CBatchProc.h"

CBatchProc::CBatchProc(string strMeta, string strPrefix)
{
    // initialise

    m_strMeta = strMeta;
    m_strPrefix = strPrefix;
    cout << "Info: initialising registration." << endl;

    m_strORI = m_strPrefix + "-ORI.tif";
    m_strORItmp = m_strPrefix + "-ORItmp.tif";
    m_strORItmptfw = m_strPrefix + "-ORItmp.tfw";
    m_strORITP = m_strPrefix + "-ORI-TP.tif";

    m_strORIRS = m_strPrefix + "-ORIRS.tif";
    m_strORIRStmp = m_strPrefix + "-ORIRStmp.tif";
    m_strORIRStmptfw = m_strPrefix + "-ORIRStmp.tfw";

    m_strORIReg = m_strPrefix + "-ORI_registered.tif";

    m_strRef = m_strPrefix + "-Base.tif";
    m_strRefRS = m_strPrefix + "-BaseRS.tif";
    m_strRefRStmp = m_strPrefix + "-BaseRStmp.tif";
    m_strRefRStmptfw = m_strPrefix + "-BaseRStmp.tfw";

    m_strRefTP = m_strPrefix + "-Base-TP.tif";

    m_strDTM = m_strPrefix + "-DTM.tif";
    m_strDTMtmp = m_strPrefix + "-DTMtmp.tif";
    m_strDTMtmptfw = m_strPrefix + "-DTMtmp.tfw";
    m_strDTMReg = m_strPrefix + "-DTM_registered.tif";



    //check data exists
    if (!validateProjParam()){
        cerr << "ERROR (2): The input files cannot be validated." << endl;
        exit(1);
    }

    //Check all inputs
    if (!validateProjInputs()){
        cerr << "ERROR (2): The project input files not in correct format." << endl;
        exit(1);
    }
    cout << "Info: project input checking completed." << endl;

}

CBatchProc::~CBatchProc(){
}

bool CBatchProc::validateProjParam(){
    bool bRes = true;
    QDir qdir;
    if (!qdir.exists(m_strORI.c_str())){
        cerr << "ERROR (1): target image for co-registration does not exist." << endl;
        bRes = false;
    }

    if (!qdir.exists(m_strRef.c_str())){
        cerr << "ERROR (1): The base image does not exist." << endl;
        bRes = false;
    }

    if (!qdir.exists(m_strDTM.c_str())){
        cerr << "ERROR (1): the corresponding DTM file does not exist." << endl;
        bRes = false;
    }

    return bRes;
}

bool CBatchProc::validateProjInputs(){
    bool bRes = true;

    cout << "Info: reprojecting the reference image to Equirectangular ..." << endl;
    ostringstream strCmd1;
    strCmd1 << "gdalwarp -t_srs '+proj=eqc +lat_ts=0 +lat_0=0 +lon_0=0 +x_0=0 +y_0=0 "
             << "+a=3396000 +b=3396000 +units=m +no_defs' -r cubic "
             << "-srcnodata 0 -dstnodata 0 -tr 50 50 "
             << m_strRef << " " << m_strRefRS;
    system(strCmd1.str().c_str());

    ostringstream strCmd2;
    strCmd2 << "gdal_translate -of GTiff -co TFW=YES " << m_strRefRS << " " << m_strRefRStmp; //-BaseRStmp.tif & -BaseRStmp.tfw
    system(strCmd2.str().c_str());

    ostringstream strCmd3;
    strCmd3 << "gdalwarp -t_srs '+proj=eqc +lat_ts=0 +lat_0=0 +lon_0=0 +x_0=0 +y_0=0 "
             << "+a=3396000 +b=3396000 +units=m +no_defs' -r cubic "
             << "-srcnodata 0 -dstnodata 0 -tr 50 50 "
             << m_strORI << " " << m_strORIRS;
    system(strCmd3.str().c_str());

    ostringstream strCmd4;
    strCmd4 << "gdal_translate -of GTiff -co TFW=YES " << m_strORIRS << " " << m_strORIRStmp; //-ORIRStmp.tif & -ORIRStmp.tfw
    system(strCmd4.str().c_str());


    cout << "Info: Reading in " << m_strORIRStmptfw << endl;
    ifstream sfORIRStmptfw;
    sfORIRStmptfw.open(m_strORIRStmptfw.c_str());
    cout << "Info: loading ORI tfw." << endl;
    if (sfORIRStmptfw.is_open()){
        float m_fTmp;
        sfORIRStmptfw >> m_fORIRESx;
        sfORIRStmptfw >> m_fTmp;
        sfORIRStmptfw >> m_fTmp;
        sfORIRStmptfw >> m_fORIRESy;
        sfORIRStmptfw >> m_fORIULx;
        sfORIRStmptfw >> m_fORIULy;

        sfORIRStmptfw.close();
    }


    else {
        bRes = false;
        cout << "ERROR (1): cannot read in " << m_strORIRStmptfw << endl;
    }


    cout << "Info: (original ORI image) upper left x is " << m_fORIULx << "; upper left y is " << m_fORIULy
         << "; x resolution is " << m_fORIRESx << "; y resolution is " << m_fORIRESy << endl;


    cout << "Info: Reading in " << m_strRefRStmptfw << endl;
    ifstream sfRefRStmptfw;
    sfRefRStmptfw.open(m_strRefRStmptfw.c_str());
    cout << "Info: loading the base tfw." << endl;
    if (sfRefRStmptfw.is_open()){
        float m_fTmp;
        sfRefRStmptfw >> m_fRefRESx;
        sfRefRStmptfw >> m_fTmp;
        sfRefRStmptfw >> m_fTmp;
        sfRefRStmptfw >> m_fRefRESy;
        sfRefRStmptfw >> m_fRefULx;
        sfRefRStmptfw >> m_fRefULy;

        sfRefRStmptfw.close();
    }


    else {
        bRes = false;
        cout << "ERROR (1): cannot read in " << m_strRefRStmptfw << endl;
    }

    cout << "Info: (original base image) upper left x is " << m_fRefULx << "; upper left y is " << m_fRefULy
         << "; x resolution is " << m_fRefRESx << "; y resolution is " << m_fRefRESy << endl;

    cout << "Info: loading target and reference images ..." << endl;
    m_mORI = imread(m_strORIRS, CV_LOAD_IMAGE_ANYDEPTH);
    m_mRef = imread(m_strRefRS, CV_LOAD_IMAGE_ANYDEPTH);

    if (m_mORI.depth() != 0){
        bRes = false;
        cout << "ERROR (1): the ORI image is not in uint8. Please check the previous conversion step." << endl;
    }

    if (m_mRef.depth() != 0){
        bRes = false;
        cout << "ERROR (1): the base (reference) image is not in uint8. Please check the previous conversion step." << endl;
    }

    return bRes;
}

void CBatchProc::doBatchProcessing(){

    char buff_start[20];
    time_t now_start = time(NULL);
    strftime(buff_start, 20, "%Y-%m-%dT%H:%M", localtime(&now_start));
    m_strStartTime.clear();
    m_strStartTime =  buff_start;

    cout << "Info: co-registration has started [ " << m_strStartTime << " ]." << endl;
    doCorregistration ();

    char buff_end[20];
    time_t now_end = time(NULL);
    strftime(buff_end, 20, "%Y-%m-%dT%H:%M", localtime(&now_end));
    m_strEndTime.clear();
    m_strEndTime = buff_end;

    cout << "Info: co-registration has completed [ " << m_strEndTime << " ]." << endl;

    cout <<  "Info: Clearing up temporary files ..." << endl;

    ostringstream strCmd9;
    strCmd9 << "rm " << m_strORItmp << " " << m_strORItmptfw << " " << m_strORIRStmp << " "
            << m_strORIRStmptfw << " " << m_strRefRStmp << " " << m_strRefRStmptfw << " "
            << m_strDTMtmp << " " << m_strDTMtmptfw;
    system(strCmd9.str().c_str());
    cout << endl << endl;

}

void CBatchProc::doCorregistration(){

    //FileStorage fs(m_strMeta, FileStorage::READ);
    //FileNode tl = fs["MSASURFParam"];
    //int m_nMinHessian = (int)tl["nMinHessian"];

    std::vector<KeyPoint> keypoints_1, keypoints_2;
    Mat descriptors_1, descriptors_2;
    Ptr<FeatureDetector> detector = ORB::create();

    Ptr<DescriptorExtractor> descriptor = ORB::create();

    Ptr<DescriptorMatcher> matcher  = DescriptorMatcher::create ( "BruteForce-Hamming" );

    cout <<  "Info: Detecting Oriented FAST ..." << endl;
    detector->detect ( m_mORI, keypoints_1 );
    detector->detect ( m_mRef, keypoints_2 );

    cout <<  "Info: Deriving BRIEF descriptors ..." << endl;
    descriptor->compute ( m_mORI, keypoints_1, descriptors_1 );
    descriptor->compute ( m_mRef, keypoints_2, descriptors_2 );

    cout <<  "Info: Matching BRIEF descriptors ..." << endl;
    vector<DMatch> matches;
    matcher->match ( descriptors_1, descriptors_2, matches );

    Mat m_mORITP = Mat::zeros(m_mORI.size(), CV_8UC3);
    Mat m_mRefTP = Mat::zeros(m_mRef.size(), CV_8UC3);

    drawKeypoints(m_mORI, keypoints_1, m_mORITP);
    drawKeypoints(m_mRef, keypoints_2, m_mRefTP);

    imwrite(m_strORITP, m_mORITP);
    imwrite(m_strRefTP, m_mRefTP);

    double min_dist=10000, max_dist=0;

    cout <<  "Info: Finding min/max distances of all matches ..." << endl;
    for ( int i = 0; i < descriptors_1.rows; i++ ){
        double dist = matches[i].distance;
        if ( dist < min_dist ) min_dist = dist;
        if ( dist > max_dist ) max_dist = dist;
    }
    cout <<  "Info: Discarding bad matches ..." << endl;

    vector<DMatch> good_matches;
    for ( int i = 0; i < descriptors_1.rows; i++ ){
        if ( matches[i].distance <= max ( 2*min_dist, 30.0 ) ){
            good_matches.push_back ( matches[i] );
        }
    }

    vector<float> m_fvULx_updated, m_fvULy_updated;

    cout <<  "Info: Calculating x/y adjustments ..." << endl;

    for(vector<DMatch>::size_type i=0; i<good_matches.size(); i++){
        float fORIx = keypoints_1[good_matches[i].queryIdx].pt.x;
        float fORIy = keypoints_1[good_matches[i].queryIdx].pt.y;

        float fRefx = keypoints_2[good_matches[i].trainIdx].pt.x;
        float fRefy = keypoints_2[good_matches[i].trainIdx].pt.y;

        float m_fULx_updated = m_fORIULx + m_fRefULx + m_fRefRESx*fRefx - (m_fORIULx + m_fORIRESx*fORIx);
        float m_fULy_updated = m_fORIULy + m_fRefULy + m_fRefRESy*fRefy - (m_fORIULy + m_fORIRESy*fORIy);

        m_fvULx_updated.push_back(m_fULx_updated);
        m_fvULy_updated.push_back(m_fULy_updated);

    }

    sort(m_fvULx_updated.begin(), m_fvULx_updated.end());
    float m_fORIULx_updated = m_fvULx_updated[m_fvULx_updated.size() / 2];
    sort(m_fvULy_updated.begin(), m_fvULy_updated.end());
    float m_fORIULy_updated = m_fvULy_updated[m_fvULy_updated.size() / 2];

    Mat mORI = imread(m_strORI, CV_LOAD_IMAGE_ANYDEPTH);
    Mat mDTM = imread(m_strDTM, CV_LOAD_IMAGE_ANYDEPTH);

    ostringstream strCmd5;
    strCmd5 << "gdal_translate -of GTiff -co TFW=YES " << m_strORI << " " << m_strORItmp; //-ORItmp.tif & -ORItmp.tfw
    system(strCmd5.str().c_str());

    ostringstream strCmd6;
    strCmd6 << "gdal_translate -of GTiff -co TFW=YES " << m_strDTM << " " << m_strDTMtmp; //-ORItmp.tif & -ORItmp.tfw
    system(strCmd6.str().c_str());

    float fORIresX, fORIresY, fDTMresX, fDTMresY;

    ifstream sfORItmptfw;
    sfORItmptfw.open(m_strORItmptfw.c_str());
    cout << "Info: loading ORI tfw." << endl;
    if (sfORItmptfw.is_open()){
        float m_fTmp;
        sfORItmptfw >> fORIresX;
        sfORItmptfw >> m_fTmp;
        sfORItmptfw >> m_fTmp;
        sfORItmptfw >> fORIresY;
        sfORItmptfw >> m_fTmp;
        sfORItmptfw >> m_fTmp;

        sfORItmptfw.close();
    }

    else {
        cout << "ERROR (1): cannot read in " << m_strORItmptfw << endl;
    }

    cout << "Info: Reading in " << m_strDTMtmptfw << endl;
    ifstream sfDTMtmptfw;
    sfDTMtmptfw.open(m_strDTMtmptfw.c_str());
    cout << "Info: loading DTM tfw." << endl;
    if (sfDTMtmptfw.is_open()){
        float m_fTmp;
        sfDTMtmptfw >> fDTMresX;
        sfDTMtmptfw >> m_fTmp;
        sfDTMtmptfw >> m_fTmp;
        sfDTMtmptfw >> fDTMresY;
        sfDTMtmptfw >> m_fTmp;
        sfDTMtmptfw >> m_fTmp;

        sfDTMtmptfw.close();
    }

    else {

        cout << "ERROR (1): cannot read in " << m_strDTMtmptfw << endl;
    }

    float m_fORILRx_updated = m_fORIULx_updated + fORIresX*mORI.cols;
    float m_fORILRy_updated = m_fORIULy_updated + fORIresY*mORI.rows;

    float m_fDTMULx_updated = m_fORIULx_updated;
    float m_fDTMULy_updated = m_fORIULy_updated;
    float m_fDTMLRx_updated = m_fDTMULx_updated + fDTMresX*mDTM.cols;
    float m_fDTMLRy_updated = m_fDTMULy_updated + fDTMresY*mDTM.rows;

    cout <<  "Info: Calculating x/y adjustments ..." << endl;

    ostringstream strCmd7;
    strCmd7 << "gdal_translate -of GTiff -a_ullr " << m_fORIULx_updated << " " << m_fORIULy_updated << " "
            << m_fORILRx_updated << " " << m_fORILRy_updated << " "
            << m_strORI << " " << m_strORIReg;
    system(strCmd7.str().c_str());

    ostringstream strCmd8;
    strCmd8 << "gdal_translate -of GTiff -a_ullr " << m_fDTMULx_updated << " " << m_fDTMULy_updated << " "
            << m_fDTMLRx_updated << " " << m_fDTMLRy_updated << " "
            << m_strDTM << " " << m_strDTMReg;
    system(strCmd8.str().c_str());

    cout << "Info: (updated ORI/DTM image) upper left x is " << m_fORIULx_updated << "; upper left y is " << m_fORIULy_updated << ". " << endl;

}
