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
#include <openssl/md5.h>
#ifdef __cplusplus
}
#endif

#include <iostream>
#include <fstream> 
#include <iomanip>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

class Digest
{
public:
    Digest();
    virtual ~Digest();

    /**
     * md5 algorithm
     * @param outFormat 2代表二进制 out参数16字节
     *                 16代表16进制表示形式 out参数32字节
     * @param in 输入数据
     * @param inLen 输入数据的长度
     * @param out 输出的结果
     * @return true 成功, false失败
     */
    bool md5(int outFormat, unsigned char *in, int inLen, unsigned char *out);

    /**
     * sha1 algorithm
     * @param outFormat 2代表二进制 out参数20字节
     *                 16代表16进制表示形式 out参数40字节
     * @param in 输入数据
     * @param inLen 输入数据的长度
     * @param out 输出的结果
     * @return true 成功, false失败
     */
    bool sha1(int outFormat, unsigned char *in, int inLen, unsigned char *out);

    /**
     * sha224 algorithm
     * @param outFormat 2代表二进制 out参数28字节
     *                 16代表16进制表示形式 out参数56字节
     * @param in 输入数据
     * @param inLen 输入数据的长度
     * @param out 输出的结果
     * @return true 成功, false失败
     */
    bool sha224(int outFormat, unsigned char *in, int inLen, unsigned char *out);

    /**
     * sha256 algorithm
     * @param outFormat 2代表二进制 out参数32字节
     *                 16代表16进制表示形式 out参数64字节
     * @param in 输入数据
     * @param inLen 输入数据的长度
     * @param out 输出的结果
     * @return true 成功, false失败
     */
    bool sha256(int outFormat, unsigned char *in, int inLen, unsigned char *out);

    /**
     * sha384 algorithm
     * @param outFormat 2代表二进制 out参数48字节
     *                 16代表16进制表示形式 out参数96字节
     * @param in 输入数据
     * @param inLen 输入数据的长度
     * @param out 输出的结果
     * @return true 成功, false失败
     */
    bool sha384(int outFormat, unsigned char *in, int inLen, unsigned char *out);

    /**
     * sha512 algorithm
     * @param outFormat 2代表二进制 out参数64字节
     *                 16代表16进制表示形式 out参数128字节
     * @param in 输入数据
     * @param inLen 输入数据的长度
     * @param out 输出的结果
     * @return true 成功, false失败
     */
    bool sha512(int outFormat, unsigned char *in, int inLen, unsigned char *out);
};
