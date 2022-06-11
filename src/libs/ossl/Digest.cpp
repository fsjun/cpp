
#include "Digest.h"

Digest::Digest()
{

}

Digest::~Digest()
{

}

bool Digest::md5(int outFormat, unsigned char *in, int inLen, unsigned char *out)
{
    unsigned char md[MD5_DIGEST_LENGTH] = {};
    if (nullptr == out)
    {
        return false;
    }
    MD5(in, inLen, md);
    if (2 == outFormat)
    {
        memcpy(out, md, sizeof(md));
    }
    else if (16 == outFormat)
    {
        std::ostringstream oss;
        for (int i=0; i<sizeof(md); i++)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)md[i];
        }
        std::string str = oss.str();
        memcpy(out,str.c_str(),str.length());
    }
    return true;
}

bool Digest::sha1(int outFormat, unsigned char *in, int inLen, unsigned char *out)
{
    unsigned char md[SHA_DIGEST_LENGTH] = {};
    if (nullptr == out)
    {
        return false;
    }
    SHA1(in, inLen, md);
    if (2 == outFormat)
    {
        memcpy(out, md, sizeof(md));
    }
    else if (16 == outFormat)
    {
        std::ostringstream oss;
        for (int i=0; i<sizeof(md); i++)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)md[i];
        }
        std::string str = oss.str();
        memcpy(out,str.c_str(),str.length());
    }
    return true;
}

bool Digest::sha224(int outFormat, unsigned char *in, int inLen, unsigned char *out)
{
    unsigned char md[SHA224_DIGEST_LENGTH] = {};
    if (nullptr == out)
    {
        return false;
    }
    SHA224(in, inLen, md);
    if (2 == outFormat)
    {
        memcpy(out, md, sizeof(md));
    }
    else if (16 == outFormat)
    {
        std::ostringstream oss;
        for (int i=0; i<sizeof(md); i++)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)md[i];
        }
        std::string str = oss.str();
        memcpy(out,str.c_str(),str.length());
    }
    return true;
}

bool Digest::sha256(int outFormat, unsigned char *in, int inLen, unsigned char *out)
{
    unsigned char md[SHA256_DIGEST_LENGTH] = {};
    if (nullptr == out)
    {
        return false;
    }
    SHA256(in, inLen, md);
    if (2 == outFormat)
    {
        memcpy(out, md, sizeof(md));
    }
    else if (16 == outFormat)
    {
        std::ostringstream oss;
        for (int i=0; i<sizeof(md); i++)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)md[i];
        }
        std::string str = oss.str();
        memcpy(out,str.c_str(),str.length());
    }
    return true;
}

bool Digest::sha384(int outFormat, unsigned char *in, int inLen, unsigned char *out)
{
    unsigned char md[SHA384_DIGEST_LENGTH] = {};
    if (nullptr == out)
    {
        return false;
    }
    SHA384(in, inLen, md);
    if (2 == outFormat)
    {
        memcpy(out, md, sizeof(md));
    }
    else if (16 == outFormat)
    {
        std::ostringstream oss;
        for (int i=0; i<sizeof(md); i++)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)md[i];
        }
        std::string str = oss.str();
        memcpy(out,str.c_str(),str.length());
    }
    return true;
}

bool Digest::sha512(int outFormat, unsigned char *in, int inLen, unsigned char *out)
{
    unsigned char md[SHA512_DIGEST_LENGTH] = {};
    if (nullptr == out)
    {
        return false;
    }
    SHA512(in, inLen, md);
    if (2 == outFormat)
    {
        memcpy(out, md, sizeof(md));
    }
    else if (16 == outFormat)
    {
        std::ostringstream oss;
        for (int i=0; i<sizeof(md); i++)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)md[i];
        }
        std::string str = oss.str();
        memcpy(out,str.c_str(),str.length());
    }
    return true;
}
