#pragma once
#include "tools/cpp_common.h"
#include "threadpool/DelayedThread.h"
#include <vector>

class Distribution : public DelayedThread {
public:
    virtual ~Distribution();
    void setExpire(int val);
    void setTimeout(long val);
    void setAddr(string val);
    void setHttp(string val);
    void setReadPrefix(string val);
    void setWritePrefix(string val);
    int start();
    string getPrefixAddr();
    void enableWrite();
    void disableWrite();

private:
    void processFunc();
    void process();

    void refreshWriteKey();
    void refreshReadKey();
    void mergePrefixAddr(vector<string>& addrVec);

private:
    int mExpire = 10;
    long mTimeout = 5000;

    string mAddr;
    string mHttp;
    string mWritePrefix;
    bool mEnableWrite = true;

    string mReadPrefix;
    std::mutex mMutex;
    std::vector<string> mAddrVec;
    int mIndex = 0;
    bool mJavaCompatible = true;
};
