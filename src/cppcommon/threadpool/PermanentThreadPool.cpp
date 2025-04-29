#include "PermanentThreadPool.h"
#include "osinfo/OsSignal.h"
#include "osinfo/ThreadInfo.h"
#include "osinfo/TraceInfo.h"
#include "threadpool/ThreadPoolBase.h"

PermanentThreadPool::PermanentThreadPool(int maxSize, int queueMaxSize) : ThreadPoolBase(0, maxSize, queueMaxSize)
{
}

PermanentThreadPool::PermanentThreadPool(int minSize, int maxSize, int queueMaxSize) : ThreadPoolBase(minSize, maxSize, queueMaxSize)
{
}

PermanentThreadPool::~PermanentThreadPool()
{
}

int PermanentThreadPool::startThread(string stateId)
{
    std::unique_lock<mutex> l(mMutex);
    mOwner = ThreadInfo::GetTid();
    if (!mRunning) {
        mOwner = 0;
        return -1;
    }
    auto it = std::ranges::find_if(mWorkerThreads, [stateId](auto val){ return val->getId() == stateId; });
    if (it != mWorkerThreads.end()) {
        mOwner = 0;
        WARNLN("permanent thread is exist when start thread, stateId:{}", stateId);
        return -1;
    }
    if (!mIdleThreads.empty()) {
        auto thread = mIdleThreads.front();
        mIdleThreads.pop_front();
        thread->setId(stateId);
        mWorkerThreads.emplace_back(thread);
        return 0;
    }
    int size = mWorkerThreads.size() + mIdleThreads.size();
    if (mMaxSize > 0 && size >= mMaxSize) {
        mOwner = 0;
        ERR("permanent thread pool exceed max, current:{} max {}\n", size, mMaxSize);
        return -1;
    }
    auto thread = ThreadPoolBase::startThread();
    thread->setId(stateId);
    mWorkerThreads.emplace_back(thread);
    mOwner = 0;
    return 0;
}

void PermanentThreadPool::stopThread(string stateId)
{
    std::unique_lock<mutex> l(mMutex);
    mOwner = ThreadInfo::GetTid();
    if (!mRunning) {
        mOwner = 0;
        return;
    }
    auto it = std::ranges::find_if(mWorkerThreads, [stateId](auto val){ return val->getId() == stateId; });
    if (it == mWorkerThreads.end()) {
        mOwner = 0;
        WARN("permanent thread is not exist when stop thread, stateId:{}\n", stateId.c_str());
        return;
    }
    auto thread = *it;
    std::erase(mWorkerThreads, thread);
    int size = mWorkerThreads.size() + mIdleThreads.size();
    if (size < mMinSize) {
        thread->setId("");
        mIdleThreads.emplace_back(thread);
    } else {
        thread->stop();
    }
    mOwner = 0;
}

int PermanentThreadPool::execute(string stateId, function<void()> task)
{
    std::unique_lock<mutex> l(mMutex);
    mOwner = ThreadInfo::GetTid();
    if (!mRunning) {
        mOwner = 0;
        return -1;
    }
    auto it = std::ranges::find_if(mWorkerThreads, [stateId](auto val){ return val->getId() == stateId; });
    if (it == mWorkerThreads.end()) {
        mOwner = 0;
        WARNLN("permanent thread is not exist when execute, stateId:{}", stateId);
        return -1;
    }
    auto thread = *it;
    thread->execute(task);
    mOwner = 0;
    return 0;
}
