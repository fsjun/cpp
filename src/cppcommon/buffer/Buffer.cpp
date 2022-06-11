#include "Buffer.h"
#include <iostream>

using namespace std;

Buffer::Buffer(int size)
{
    mBuffer = new char[size + 1]();
    mBufferLen = size + 1;
    mBufferStart = mBuffer;
    mBufferEnd = mBuffer;
}

Buffer::~Buffer()
{
    delete[] mBuffer;
}

bool Buffer::isEmpty()
{
    int used = getUsedSize();
    return used == 0;
}

bool Buffer::isFull()
{
    int used = getUsedSize();
    return used == (mBufferLen - 1);
}

int Buffer::getUsedSize()
{
    int used = mBufferEnd - mBufferStart;
    if (used < 0) {
        used += mBufferLen;
    }
    return used;
}

int Buffer::capacity()
{
    return mBufferLen - 1;
}

int Buffer::cut(char* buf, int len)
{
    int used = getUsedSize();
    if (len > used) {
        return -1;
    }
    // if (*mBufferStart == 0 && mBufferStart != mBufferEnd)
    // {
    //     cout << "======error occur====" << endl;
    // }
    if (mBufferStart <= mBufferEnd) {
        if (buf) {
            memcpy(buf, mBufferStart, len);
        }
        memset(mBufferStart, 0, len);
        mBufferStart += len;
    } else {
        int endLen = mBufferEnd - mBuffer;
        int startLen = used - endLen;
        if (startLen > len) {
            if (buf) {
                memcpy(buf, mBufferStart, len);
            }
            memset(mBufferStart, 0, len);
            mBufferStart += len;
        } else {
            endLen = len - startLen;
            if (buf) {
                memcpy(buf, mBufferStart, startLen);
            }
            memset(mBufferStart, 0, startLen);
            if (endLen > 0) {
                if (buf) {
                    memcpy(buf + startLen, mBuffer, endLen);
                }
                memset(mBuffer, 0, endLen);
            }
            mBufferStart = mBuffer + endLen;
        }
    }
    // if (*mBufferStart == 0 && mBufferStart != mBufferEnd)
    // {
    //     cout << "======error occur====" << endl;
    // }
    return len;
}

int Buffer::copy(char* buf, int len)
{
    int used = getUsedSize();
    if (len > used) {
        return -1;
    }
    // if (*mBufferStart == 0 && mBufferStart != mBufferEnd)
    // {
    //     cout << "======error occur====" << endl;
    // }
    if (mBufferStart <= mBufferEnd) {
        if (buf) {
            memcpy(buf, mBufferStart, len);
        }
    } else {
        int endLen = mBufferEnd - mBuffer;
        int startLen = used - endLen;
        if (startLen > len) {
            if (buf) {
                memcpy(buf, mBufferStart, len);
            }
        } else {
            endLen = len - startLen;
            if (buf) {
                memcpy(buf, mBufferStart, startLen);
            }
            if (endLen > 0) {
                if (buf) {
                    memcpy(buf + startLen, mBuffer, endLen);
                }
            }
        }
    }
    // if (*mBufferStart == 0 && mBufferStart != mBufferEnd)
    // {
    //     cout << "======error occur====" << endl;
    // }
    return len;
}

int Buffer::appendCover(char* buf, int len)
{
    int used = getUsedSize();
    int freeSize = mBufferLen - used - 1;

    if (len <= freeSize) {
        append(buf, len);
        return len;
    }
    int ret = append(buf, freeSize);
    if (ret < 0) {
        return -1;
    }
    int leftSize = len - freeSize;
    ret = cut(nullptr, leftSize);
    if (ret < 0) {
        return -1;
    }
    ret = append(buf + freeSize, leftSize);
    if (ret < 0) {
        return -1;
    }
    return len;
}

int Buffer::append(char* buf, int len)
{
    int used = getUsedSize();
    int freeSize = mBufferLen - used - 1;

    if (len > freeSize) {
        return -1;
    }
    // if (*mBufferStart == 0 && mBufferStart != mBufferEnd)
    // {
    //     cout << "======error occur====" << endl;
    // }
    if (mBufferStart > mBufferEnd) {
        memcpy(mBufferEnd, buf, len);
        mBufferEnd += len;
    } else {
        int startLen = mBufferStart - mBuffer - 1;
        int endLen = freeSize - startLen;
        if (len < endLen) {
            memcpy(mBufferEnd, buf, len);
            mBufferEnd += len;
        } else {
            startLen = len - endLen;
            memcpy(mBufferEnd, buf, endLen);
            if (startLen > 0) {
                memcpy(mBuffer, buf + endLen, startLen);
            }
            mBufferEnd = mBuffer + startLen;
        }
    }
    // if (*mBufferStart == 0 && mBufferStart != mBufferEnd)
    // {
    //     cout << "======error occur====" << endl;
    // }
    return len;
}

int Buffer::fill(int val, int len)
{
    int used = getUsedSize();
    int freeSize = mBufferLen - used - 1;

    if (len > freeSize) {
        return -1;
    }
    // if (*mBufferStart == 0 && mBufferStart != mBufferEnd)
    // {
    //     cout << "======error occur====" << endl;
    // }
    if (mBufferStart > mBufferEnd) {
        memset(mBufferEnd, val, len);
        mBufferEnd += len;
    } else {
        int startLen = mBufferStart - mBuffer - 1;
        int endLen = freeSize - startLen;
        if (len < endLen) {
            memset(mBufferEnd, val, len);
            mBufferEnd += len;
        } else {
            startLen = len - endLen;
            memset(mBufferEnd, val, endLen);
            if (startLen > 0) {
                memset(mBuffer, val, startLen);
            }
            mBufferEnd = mBuffer + startLen;
        }
    }
    // if (*mBufferStart == 0 && mBufferStart != mBufferEnd)
    // {
    //     cout << "======error occur====" << endl;
    // }
    return len;
}

bool Buffer::matchDelimiter(char* p, char* delimiter, int delimiterLen)
{
    bool ret = true;
    while (delimiterLen--) {
        if (p >= mBuffer + mBufferLen) {
            p -= mBufferLen;
        }
        if (*p != *delimiter) {
            ret = false;
            break;
        }
        p++;
        delimiter++;
    }
    return ret;
}

char* Buffer::findDelimiter(char* delimiter, int delimiterLen)
{
    char* p = NULL;
    int used = getUsedSize();
    bool ret;
    if (used < delimiterLen) {
        return NULL;
    }
    for (int i = 0; i <= used - delimiterLen; i++) {
        ret = matchDelimiter(mBufferStart + i, delimiter, delimiterLen);
        if (ret) {
            p = mBufferStart + i;
            break;
        }
    }
    if (p >= mBuffer + mBufferLen) {
        p -= mBufferLen;
    }
    return p;
}

int Buffer::extract(char* delimiter, int delimiterLen, char*& buf, int& len)
{
    int ret = -1;
    char* p = findDelimiter(delimiter, delimiterLen);
    if (NULL != p) {
        len = p + delimiterLen - mBufferStart;
        if (len < 0) {
            len += mBufferLen;
        }
        buf = new char[len + 1]();
        ret = cut(buf, len);
        if (ret > 0) {
            ret = 0;
        }
    }
    return ret;
}
