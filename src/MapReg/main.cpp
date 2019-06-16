#include "CBatchProc.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc == 3) {

        string strMeta(argv[1]);
        string strPrefix(argv[2]);

        CBatchProc batchProc(strMeta, strPrefix);
        batchProc.doBatchProcessing();
        return 0;

    }
    else {
        cout << "ERROR (2): Wrong Input Arguments." << endl;
        cout << "USAGE: ./RegisterMap MetaFile Prefix" << endl;

        return 0;
    }
}
