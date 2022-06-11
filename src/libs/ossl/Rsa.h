#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include "OpenSSL.h"

using std::string;

typedef struct pw_cb_data {
    const void *password;
    const char *prompt_info;
} PW_CB_DATA;

class Rsa
{
public:
    Rsa();
    virtual ~Rsa();

    int pubkeyEncrypt(string keyfile, unsigned char *in, int inLen, unsigned char *&out, int &outLen);
    int prikeyDecrypt(string keyfile, string password, unsigned char *in, int inLen, unsigned char *&out, int &outLen);
    int prikeyEncrypt(string keyfile, string password, unsigned char *in, int inLen, unsigned char *&out, int &outLen);
    int pubkeyDecrypt(string keyfile, unsigned char *in, int inLen, unsigned char *&out, int &outLen);
    int pubkeyDecrypt2(string pem, unsigned char *in, int inLen, unsigned char *&out, int &outLen);
    int sign(string keyfile, string password, unsigned char *in, int inLen, unsigned char *&out, int &outLen);
    bool verify(string certfile, unsigned char *in, int inLen, unsigned char *signData, int signDataLen);
    bool pubVerify(string pem, unsigned char *in, int inLen, unsigned char *signData, int signDataLen);

private:
    static int passwordCallback(char *buf, int bufsiz, int verify, PW_CB_DATA *cb_tmp);

    RSA *getRsaFromX509(string keyfile);
    RSA *getRsaFromPrikey(string keyfile, string password);
    RSA *getRsaFromBuf(string pem);
};
