#include "threadpool/ThreadManager.h"
#include <memory>

int ThreadManager::Start()
{
    auto threadManager = ThreadManager::GetInstance();
    threadManager->start();
}

int ThreadManager::start()
{
    std::lock_guard l(mMutex);
    if (mRuning) {
        return 0;
    }
    mRuning = true;
    mThread = std::make_unique<Thread>();
    mThread->start(std::bind(&ThreadManager::thread_func, shared_from_this()));
    return 0;
}

int ThreadManager::stop()
{
    std::lock_guard l(mMutex);
    if (!mRuning) {
        return 0;
    }
    mRuning = false;
    mDeadThreadQueue.destroy();
    return 0;
}

void ThreadManager::join()
{
    if (mThread) {
        // auto join
        mThread.reset();
    }
}

bool ThreadManager::push(shared_ptr<BaseThread> val)
{
    return mDeadThreadQueue.push(val);
}

void ThreadManager::thread_func() noexcept
{
    bool ret;
    while (mRuning) {
        shared_ptr<BaseThread> t;
        ret = mDeadThreadQueue.pop(t);
        if (!ret) {
            break;
        }
        t->join();
        t.reset();
    }
}
