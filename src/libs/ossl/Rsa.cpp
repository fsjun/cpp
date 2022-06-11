#include "Rsa.h"
#include "Digest.h"

using namespace std;

Rsa::Rsa()
{

}

Rsa::~Rsa()
{

}

int Rsa::passwordCallback(char *buf, int bufsiz, int verify, PW_CB_DATA *cb_tmp)
{
    UI *ui = NULL;
    int res = 0;
    const char *password = NULL;
    PW_CB_DATA *cb_data = (PW_CB_DATA *)cb_tmp;

    if (cb_data && cb_data->password) {
        password = (const char *)cb_data->password;
        res = strlen(password);
        if (res > bufsiz)
        {
            res = bufsiz;
        }
        memcpy(buf, password, res);
        return res;
    }
    return res;
}

RSA *Rsa::getRsaFromX509(string keyfile)
{
    X509 *x = nullptr;
    BIO *b = nullptr;
    EVP_PKEY *k = nullptr;
    RSA *rsa = nullptr;

    b = BIO_new_file(keyfile.c_str(), "r");
    if (nullptr == b)
    {
        printErr();
        goto end;
    }
    x = PEM_read_bio_X509(b, NULL, NULL, NULL);
    if (nullptr == x)
    {
        printErr();
        goto end;
    }
    k = X509_get_pubkey(x);
    if (nullptr == k)
    {
        printErr();
        goto end;
    }
    rsa = EVP_PKEY_get1_RSA(k);
    if (nullptr == rsa)
    {
        printErr();
        goto end;
    }
end:
    BIO_free(b);
    X509_free(x);
    EVP_PKEY_free(k);
    return rsa;
}

RSA *Rsa::getRsaFromBuf(string pem)
{
    X509 *x = nullptr;
    BIO *b = nullptr;
    EVP_PKEY *k = nullptr;
    RSA *rsa = nullptr;

    b = BIO_new_mem_buf((void*)pem.c_str(), -1);
    if (nullptr == b)
    {
        printErr();
        goto end;
    }
    x = PEM_read_bio_X509(b, NULL, NULL, NULL);
    if (nullptr == x)
    {
        printErr();
        goto end;
    }
    k = X509_get_pubkey(x);
    if (nullptr == k)
    {
        printErr();
        goto end;
    }
    rsa = EVP_PKEY_get1_RSA(k);
    if (nullptr == rsa)
    {
        printErr();
        goto end;
    }
end:
    BIO_free(b);
    X509_free(x);
    EVP_PKEY_free(k);
    return rsa;
}

RSA *Rsa::getRsaFromPrikey(string keyfile, string password)
{
    BIO *b = nullptr;
    EVP_PKEY *k = nullptr;
    RSA *rsa = nullptr;
    PW_CB_DATA cb_data;
    int ret = 0;

    cb_data.password = password.c_str();
    b = BIO_new(BIO_s_file());
    if (!b)
    {
        printErr();
        goto end;
    }
    ret = BIO_read_filename(b, keyfile.c_str());
    if (ret <= 0)
    {
        printErr();
        goto end;
    }
    k = PEM_read_bio_PrivateKey(b, NULL,(pem_password_cb *)passwordCallback, &cb_data);
    if (!k)
    {
        printErr();
        goto end;
    }
    rsa = EVP_PKEY_get1_RSA(k);
end:
    if (b)
    {
        BIO_free(b);
    }
    if (k)
    {
        EVP_PKEY_free(k);
    }
    return rsa;
}

int Rsa::pubkeyEncrypt(string keyfile, unsigned char *in, int inLen, unsigned char *&pOut, int &pOutLen)
{
    RSA *rsa = nullptr;
    int keysize = 0;
    int blockSize = 0;
    int size = 0;
    int cnt = 0;
    int remain = 0;
    unsigned char *cipher_text = nullptr;
    unsigned char *out = nullptr;
    int outLen = 0;
    int i = 0;

    defer([&](){
        if (out)
        {
            delete out;
        }
        if (cipher_text)
        {
            delete [] cipher_text;
        }
        if (rsa)
        {
            RSA_free(rsa);
        }
    });

    rsa = getRsaFromX509(keyfile);
    if (nullptr == rsa)
    {
        return -1;
    }
    keysize = RSA_size(rsa);
    blockSize = keysize - RSA_PKCS1_PADDING_SIZE;
    cnt = inLen / blockSize;
    remain = inLen % blockSize;
    cipher_text = new unsigned char[keysize]();
    if (remain)
    {
        outLen = (cnt+1) * keysize;
    }
    else
    {
        outLen = cnt * keysize;
    }
    out = new unsigned char[outLen]();
    for (i = 0; i < cnt; i++)
    {
        // RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, RSA_SSLV23_PADDING, RSA_NO_PADDING
        size += RSA_public_encrypt(blockSize, in+i*blockSize, cipher_text, rsa, RSA_PKCS1_PADDING);
        if (size < 0)
        {
            printErr();
            return -1;
        }
        memcpy(out+i*keysize, cipher_text, keysize);
    }
    if (remain)
    {
        size += RSA_public_encrypt(inLen - i*blockSize, in + i*blockSize, cipher_text, rsa, RSA_PKCS1_PADDING);
        if (size < 0)
        {
            printErr();
            return -1;
        }
        memcpy(out+i*keysize, cipher_text, keysize);
    }
    // cout << size << " " << outLen << endl;
    pOut = out;
    out = nullptr;
    pOutLen = outLen;
    return 0;
}

int Rsa::prikeyDecrypt(string keyfile, string password, unsigned char *in, int inLen, unsigned char *&pOut, int &pOutLen)
{
    RSA *rsa = nullptr;
    int keysize = 0;
    int size = 0;
    int cnt = 0;
    unsigned char *plain_text = nullptr;
    unsigned char *out = nullptr;
    int outLen = 0;
    int len = 0;

    defer([&](){
        if (out)
        {
            delete out;
        }
        if (plain_text)
        {
            delete [] plain_text;
        }
        if (rsa)
        {
            RSA_free(rsa);
        }
    });

    rsa = getRsaFromPrikey(keyfile, password);
    keysize = RSA_size(rsa);
    cnt = inLen / keysize;
    plain_text = new unsigned char[keysize]();
    outLen = inLen;
    out = new unsigned char[outLen]();
    for (int i = 0; i < cnt; i++)
    {
        // RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, RSA_SSLV23_PADDING, RSA_NO_PADDING
        len = RSA_private_decrypt(keysize, in+i*keysize, plain_text, rsa, RSA_PKCS1_PADDING);
        if (len < 0)
        {
            return -1;
        }
        memcpy(out+size, plain_text, len);
        size += len;
    }
    // cout << size << " " << outLen << endl;
    pOut = out;
    out = nullptr;
    pOutLen = size;
    return 0;
}

int Rsa::prikeyEncrypt(string keyfile, string password, unsigned char *in, int inLen, unsigned char *&pOut, int &pOutLen)
{
    RSA *rsa = nullptr;
    int keysize = 0;
    int blockSize = 0;
    int size = 0;
    int cnt = 0;
    int remain = 0;
    unsigned char *cipher_text = nullptr;
    unsigned char *out = nullptr;
    int outLen = 0;
    int i = 0;

    defer([&](){
        if (out)
        {
            delete out;
        }
        if (cipher_text)
        {
            delete [] cipher_text;
        }
        if (rsa)
        {
            RSA_free(rsa);
        }
    });

    rsa = getRsaFromPrikey(keyfile, password);
    if (nullptr == rsa)
    {
        return -1;
    }
    keysize = RSA_size(rsa);
    blockSize = keysize - RSA_PKCS1_PADDING_SIZE;
    cnt = inLen / blockSize;
    remain = inLen % blockSize;
    cipher_text = new unsigned char[keysize]();
    if (remain)
    {
        outLen = (cnt+1) * keysize;
    }
    else
    {
        outLen = cnt * keysize;
    }
    out = new unsigned char[outLen]();
    for (i = 0; i < cnt; i++)
    {
        // RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, RSA_SSLV23_PADDING, RSA_NO_PADDING
        size += RSA_private_encrypt(blockSize, in+i*blockSize, cipher_text, rsa, RSA_PKCS1_PADDING);
        if (size < 0)
        {
            printErr();
            return -1;
        }
        memcpy(out+i*keysize, cipher_text, keysize);
    }
    if (remain)
    {
        size += RSA_private_encrypt(inLen - i*blockSize, in + i*blockSize, cipher_text, rsa, RSA_PKCS1_PADDING);
        if (size < 0)
        {
            printErr();
            return -1;
        }
        memcpy(out+i*keysize, cipher_text, keysize);
    }
    // cout << size << " " << outLen << endl;
    pOut = out;
    out = nullptr;
    pOutLen = outLen;
    return 0;
}

int Rsa::pubkeyDecrypt(string keyfile, unsigned char *in, int inLen, unsigned char *&pOut, int &pOutLen)
{
    RSA *rsa = nullptr;
    int keysize = 0;
    int size = 0;
    int cnt = 0;
    unsigned char *plain_text = nullptr;
    unsigned char *out = nullptr;
    int outLen = 0;
    int len = 0;

    defer([&](){
        if (out)
        {
            delete out;
        }
        if (plain_text)
        {
            delete [] plain_text;
        }
        if (rsa)
        {
            RSA_free(rsa);
        }
    });

    rsa = getRsaFromX509(keyfile);
    keysize = RSA_size(rsa);
    cnt = inLen / keysize;
    plain_text = new unsigned char[keysize]();
    outLen = inLen;
    out = new unsigned char[outLen]();
    for (int i = 0; i < cnt; i++)
    {
        // RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, RSA_SSLV23_PADDING, RSA_NO_PADDING
        len = RSA_public_decrypt(keysize, in+i*keysize, plain_text, rsa, RSA_PKCS1_PADDING);
        if (len < 0)
        {
            return -1;
        }
        memcpy(out+size, plain_text, len);
        size += len;
    }
    // cout << size << " " << outLen << endl;
    pOut = out;
    out = nullptr;
    pOutLen = size;
    return 0;
}

int Rsa::pubkeyDecrypt2(string pem, unsigned char *in, int inLen, unsigned char *&pOut, int &pOutLen)
{
    RSA *rsa = nullptr;
    int keysize = 0;
    int size = 0;
    int cnt = 0;
    unsigned char *plain_text = nullptr;
    unsigned char *out = nullptr;
    int outLen = 0;
    int len = 0;

    defer([&](){
        if (out)
        {
            delete out;
        }
        if (plain_text)
        {
            delete [] plain_text;
        }
        if (rsa)
        {
            RSA_free(rsa);
        }
    });

    rsa = getRsaFromBuf(pem);
    keysize = RSA_size(rsa);
    cnt = inLen / keysize;
    plain_text = new unsigned char[keysize]();
    outLen = inLen;
    out = new unsigned char[outLen]();
    for (int i = 0; i < cnt; i++)
    {
        // RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, RSA_SSLV23_PADDING, RSA_NO_PADDING
        len = RSA_public_decrypt(keysize, in+i*keysize, plain_text, rsa, RSA_PKCS1_PADDING);
        if (len < 0)
        {
            return -1;
        }
        memcpy(out+size, plain_text, len);
        size += len;
    }
    // cout << size << " " << outLen << endl;
    pOut = out;
    out = nullptr;
    pOutLen = size;
    return 0;
}

int Rsa::sign(string keyfile, string password, unsigned char *in, int inLen, unsigned char *&out, int &outLen)
{
    Digest dgt;
    unsigned char hash[20] = {0};
    int ret = 0;
    bool isSuccess = dgt.sha1(2, in, inLen, hash);
    if (!isSuccess)
    {
        return -1;
    }
    RSA* rsa = getRsaFromPrikey(keyfile, password);
    if (nullptr == rsa) {
        return -1;
    }
    unsigned int i = RSA_size(rsa);
    out = new unsigned char[i];
    ret = RSA_sign(NID_sha1, hash, 20, out, &i, rsa);
    if (ret < 0)
    {
        return -1;
    }
    outLen = i;
    return 0;
}

bool Rsa::verify(string certfile, unsigned char *in, int inLen, unsigned char *signData, int signDataLen)
{
    Digest dgt;
    unsigned char hash[20] = {0};
    int hashLen = sizeof(hash);
    int ret = 0;
    bool isSuccess = dgt.sha1(2, in, inLen, hash);
    if (!isSuccess)
    {
        return false;
    }
    RSA* rsa = getRsaFromX509(certfile);
    if (nullptr == rsa) {
        return false;
    }
    ret = RSA_verify(NID_sha1, hash, hashLen, signData, signDataLen, rsa);
    if (ret != 1) {
        return false;
    }
    return true;
}

bool Rsa::pubVerify(string pem, unsigned char *in, int inLen, unsigned char *signData, int signDataLen)
{
    Digest dgt;
    unsigned char hash[20] = {0};
    int hashLen = sizeof(hash);
    int ret = 0;
    bool isSuccess = dgt.sha1(2, in, inLen, hash);
    if (!isSuccess)
    {
        return false;
    }
    RSA* rsa = getRsaFromBuf(pem);
    if (nullptr == rsa) {
        return false;
    }
    ret = RSA_verify(NID_sha1, hash, hashLen, signData, signDataLen, rsa);
    if (ret != 1) {
        return false;
    }
    return true;
}
