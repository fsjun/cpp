#include "DrogonManager.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <drogon/drogon.h>
#include <thread>
#include "log/Log.h"

DrogonManager::~DrogonManager()
{
    INFOLN("begin drogon manager thread join");
    if (mThread && mThread->joinable()) {
        mThread->join();
    }
    INFOLN("end drogon manager thread join");
}

int DrogonManager::start()
{
    std::lock_guard<std::mutex> l(mMutex);
    if (mIsStart) {
        return 0;
    }
    mIsStart = true;
    if (mThread && mThread->joinable()) {
        mThread->join();
        mThread = nullptr;
    }
    auto weak = weak_from_this();
    mThread = std::make_unique<thread>([weak](){
        auto self = weak.lock();
        if (!self) {
            return;
        }
        self->threadFunc();
        INFOLN("DrogonManager user_count:%d", self.use_count());
    });
    return 0;
}

void DrogonManager::threadFunc()
{
    auto& app = drogon::app();
    app.disableSigtermHandling();
    app.setThreadNum(0);
    // auto loop = app.getLoop();
    // mTimeId = loop->runEvery(std::chrono::seconds(5), []{});
    INFOLN("drogon app run");
    app.run();
    INFOLN("drogon app exit");
    mIsStart = false;
}
