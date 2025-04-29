#include "ThreadPoolBase.h"
#include "osinfo/OsSignal.h"
#include "ThreadManager.h"
#include "osinfo/ThreadInfo.h"
#include "threadpool/DelayedThread.h"
#include <ranges>

ThreadPoolBase::ThreadPoolBase(int minSize, int maxSize, int queueMaxSize)
{
    mMinSize = minSize;
    mMaxSize = maxSize;
    mQueueMaxSize = queueMaxSize;
}

ThreadPoolBase::~ThreadPoolBase()
{
}

int ThreadPoolBase::start()
{
    ThreadManager::Start();
    std::lock_guard<mutex> l(mMutex);
    if (mRunning) {
        return 0;
    }
    mRunning = true;
    if (mMinSize <= 0) {
        return 0;
    }
    for (int i = 0; i < mMinSize; ++i) {
        auto thread = startThread();
        mIdleThreads.emplace_back(thread);
    }
    return 0;
}

void ThreadPoolBase::stop()
{
    std::unique_lock<mutex> l(mMutex);
    mOwner = ThreadInfo::GetTid();
    if (!mRunning) {
        return;
    }
    mRunning = false;
    for (auto& val : mIdleThreads) {
        val->stop();
    }
    mIdleThreads.clear();
    for (auto& val : mWorkerThreads) {
        val->stop();
    }
    mOwner = 0;
}

void ThreadPoolBase::join()
{
    std::unique_lock l(mMutex);
    mCv.wait(l, [this] { return mWorkerThreads.empty() && mIdleThreads.empty(); });
}

shared_ptr<DelayedThread> ThreadPoolBase::startThread()
{
    auto thread = make_shared<DelayedThread>(mQueueMaxSize);
    thread->start();
    return thread;
}

void ThreadPoolBase::startThreadTimer(shared_ptr<DelayedThread> thread)
{
    weak_ptr<DelayedThread> weak = thread;
    thread->postDelayedTask(mTimerId, mTime, [this, weak]() {
        auto thread = weak.lock();
        onThreadExpired(thread);
    });
}

void ThreadPoolBase::onThreadExpired(shared_ptr<DelayedThread> thread)
{
    std::lock_guard l(mMutex);
    if (mRunning) {
        if (!thread->expire()) {
            startThreadTimer(thread);
            return;
        }
        std::erase(mWorkerThreads, thread);
        int size = mWorkerThreads.size() + mIdleThreads.size();
        if (size < mMinSize) {
            thread->setId("");
            mIdleThreads.emplace_back(thread);
        } else {
            thread->stop();
        }
        return;
    }
    std::erase(mWorkerThreads, thread);
    if (mIdleThreads.empty() && mWorkerThreads.empty()) {
        mCv.notify_all();
    }
}
