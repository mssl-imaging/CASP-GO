#include "ctxWrapper.h"
#include "hiriseWrapper.h"
#include "c2Wrapper.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc == 3) {
        // read metafile name
        string strParam(argv[1]);
        string strInstrument(argv[2]);

        if (strInstrument == "--ctx" || strInstrument == "--CTX"){
            ctxWrapper batchProc(strParam);
            batchProc.doBatchProcessing();
        }

        else if (strInstrument == "--hirise" || strInstrument == "--HiRISE" || strInstrument == "--HIRISE"){
            hiriseWrapper batchProc(strParam);
            batchProc.doBatchProcessing();
        }

        else if (strInstrument == "--c2" || strInstrument == "-C2" || strInstrument == "--Carbonite2"){
            c2Wrapper batchProc(strParam);
            batchProc.doBatchProcessing();
        }

        else{
            cout << "Unrecognised argument. Plese specify --ctx for MRO CTX processing or" << endl
                 << "                                     --hirise for MRO HiRISE processing or" << endl
                 << "                                     --c2 for Carbonite2 processing." << endl;

        }

    }

    else {
        cout << "Wrong number of arguments!" << endl
             << "USAGE: ./CASP-GO param.xml --ctx" << endl
             << "                           --hirise" << endl
             << "                           --c2" << endl
             << "param.xml specifies all CASP-GO input directories." << endl << endl;
        return 0;
    }
}
