#pragma once

#include "tools/Singleton.h"
#include <mutex>
#include <thread>
#include <memory>

using std::thread;
using std::unique_ptr;

class DrogonManager : public Singleton<DrogonManager>, public std::enable_shared_from_this<DrogonManager> {
public:
    ~DrogonManager();
    int start();

private:
    void threadFunc();

private:
    std::mutex mMutex;
    int mIsStart = false;
    uint64_t mTimeId;

    unique_ptr<thread> mThread;
};
