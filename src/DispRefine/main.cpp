#include "CBatchProc.h"

//#include "opencv/cv.h"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;


int main(int argc, char *argv[])
{
    if (argc == 7) {
        // read metafile name
        string strMetaFile(argv[1]);
        string strLeftImagePath(argv[2]);
        string strRightImagePath(argv[3]);
        string strDisparityX(argv[4]);
        string strDisparityY(argv[5]);
        string strOutputPrefix(argv[6]);

        CBatchProc batchProc(strMetaFile, strLeftImagePath, strRightImagePath, strDisparityX, strDisparityY, strOutputPrefix);
        batchProc.doBatchProcessing();
    }
    else {
        cout << "USAGE: GotchaDisparityRefine Metafile L R Dx Dy output_prefix" << endl;
        return 0;
    }
}
