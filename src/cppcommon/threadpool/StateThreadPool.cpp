#include "StateThreadPool.h"
#include "osinfo/ThreadInfo.h"
#include "threadpool/ThreadPoolBase.h"
#include <ranges>

StateThreadPool::StateThreadPool(int maxSize, int queueMaxSize) : ThreadPoolBase(0, maxSize, queueMaxSize)
{
}

StateThreadPool::StateThreadPool(int minSize, int maxSize, int queueMaxSize) : ThreadPoolBase(minSize, maxSize, queueMaxSize)
{
}

StateThreadPool::~StateThreadPool()
{
}

int StateThreadPool::execute(string stateId, function<void()> task)
{
    std::unique_lock<mutex> l(mMutex);
    mOwner = ThreadInfo::GetTid();
    if (!mRunning) {
        mOwner = 0;
        return -1;
    }
    auto it = std::ranges::find_if(mWorkerThreads, [stateId](auto val){ return val->getId() == stateId; });
    if (it != mWorkerThreads.end()) {
        auto thread = *it;
        thread->execute(task);
        mOwner = 0;
        return 0;
    }
    if (!mIdleThreads.empty()) {
        auto thread = mIdleThreads.front();
        mIdleThreads.pop_front();
        thread->setId(stateId);
        mWorkerThreads.emplace_back(thread);
        thread->execute(task);
        startThreadTimer(thread);
        return 0;
    }
    int size = mWorkerThreads.size() + mIdleThreads.size();
    if (mMaxSize > 0 && size >= mMaxSize) {
        mOwner = 0;
        ERRLN("state thread pool exceed max, current:{} max {}", size, mMaxSize);
        return -1;
    }
    auto thread = startThread();
    thread->setId(stateId);
    mWorkerThreads.emplace_back(thread);
    thread->execute(task);
    startThreadTimer(thread);
    return 0;
}
