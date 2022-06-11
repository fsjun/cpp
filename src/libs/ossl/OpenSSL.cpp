
#include "OpenSSL.h"

using namespace std;

void initOpenssl()
{
    ERR_load_ERR_strings();
    ERR_load_crypto_strings();
    // CRYPTO_malloc_init();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();    
}

void printErr()
{
    unsigned long ulErr = ERR_get_error();
    char szErrMsg[1024] = {0};
    char *pTmp = NULL;
    pTmp = ERR_error_string(ulErr,szErrMsg);
    cout << pTmp << endl;
}
