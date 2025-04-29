#include "ThreadPool.h"
#include "ThreadManager.h"
#include "threadpool/DelayedThread.h"
#include <algorithm>
#include <mutex>
#include <vector>

ThreadPool::ThreadPool(int maxSize, int queueMaxSize) : ThreadPoolBase(0, maxSize, queueMaxSize)
{
}

ThreadPool::ThreadPool(int minSize, int maxSize, int queueMaxSize) : ThreadPoolBase(minSize, maxSize, queueMaxSize)
{
}

ThreadPool::~ThreadPool()
{
}

int ThreadPool::execute(function<void()> task)
{
    std::lock_guard<mutex> l(mMutex);
    if (!mRunning) {
        return -1;
    }
    if (!mIdleThreads.empty()) {
        auto thread = mIdleThreads.front();
        mIdleThreads.pop_front();
        mWorkerThreads.emplace_back(thread);
        thread->execute(task);
        startThreadTimer(thread);
        return 0;
    }
    if (mIdleThreads.size() + mWorkerThreads.size() < mMaxSize) {
        auto thread = startThread();
        mWorkerThreads.emplace_back(thread);
        thread->execute(task);
        startThreadTimer(thread);
        return 0;
    }
    auto it = std::min_element(mWorkerThreads.begin(), mWorkerThreads.end(), [](auto val1, auto val2) {
        return val1->size() < val2->size();
    });
    if (it != mWorkerThreads.end()) {
        auto thread = *it;
        thread->execute(task);
        return 0;
    }
    return -1;
}
