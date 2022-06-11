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
#include <iostream>
#include "tools/common.h"

void initOpenssl();
void printErr();