#include "ThreadPoolPermanent.h"
#include "osinfo/OsSignal.h"
#include "osinfo/ThreadInfo.h"
#include "osinfo/TraceInfo.h"
#include "threadpool/ThreadPoolBase.h"

ThreadPoolPermanent::ThreadPoolPermanent(int maxSize, int queueMaxSize) : ThreadPoolBase(0, maxSize, queueMaxSize)
{
}

ThreadPoolPermanent::ThreadPoolPermanent(int minSize, int maxSize, int queueMaxSize) : ThreadPoolBase(minSize, maxSize, queueMaxSize)
{
}

ThreadPoolPermanent::~ThreadPoolPermanent()
{
}

int ThreadPoolPermanent::startThread(string stateId)
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
        ERRLN("permanent thread pool exceed max, current:{} max {}", size, mMaxSize);
        return -1;
    }
    auto thread = ThreadPoolBase::startThread();
    thread->setId(stateId);
    mWorkerThreads.emplace_back(thread);
    mOwner = 0;
    return 0;
}

void ThreadPoolPermanent::stopThread(string stateId)
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
        WARNLN("permanent thread is not exist when stop thread, stateId:{}", stateId.c_str());
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

int ThreadPoolPermanent::execute(string stateId, function<void()> task)
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
