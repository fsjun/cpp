#include "ThreadPool.h"
#include "osinfo/OsSignal.h"
#include "osinfo/TraceInfo.h"

ThreadPool::ThreadPool(int maxSize, int queueSize)
    : mDeadThreadQueue(maxSize)
    , mTaskQueue(queueSize)
{
    OsSignal::Install();
    mMaxThreadSize = maxSize;
    mQueueSize = queueSize;
    mDeadThread.reset(new Thread());
    mDeadThread->start([this]() noexcept {
        while (!mQuit) {
            string uuid;
            bool ret = mDeadThreadQueue.pop(uuid);
            if (!ret) {
                continue;
            }
            std::unique_lock<mutex> l(mMtx);
            if (mQuit) {
                break;
            }
            auto it = mThreads.find(uuid);
            if (it == mThreads.end()) {
                continue;
            }
            unique_ptr<Thread> thread = std::move(it->second);
            mThreads.erase(uuid);
            l.unlock();
            thread.reset();
        }
    });
}

ThreadPool::~ThreadPool()
{
}

int ThreadPool::execute(function<void()> task)
{
    std::lock_guard<mutex> l(mMtx);
    if (mQuit) {
        return -1;
    }
    mTaskQueue.push(task);
    if (mCurrThreadSize < mMaxThreadSize) {
        startThread(std::bind(&ThreadPool::workerThreadFunc, this));
    }
    return 0;
}

void ThreadPool::startThread(function<void()> thread_func)
{
    ++mCurrThreadSize;
    string uuid = Thread::GenUUID();
    auto it = mThreads.emplace(uuid, unique_ptr<Thread>(new Thread()));
    it.first->second->start([this, thread_func, uuid]() noexcept {
        thread_func();
        clearThread();
        mDeadThreadQueue.push(uuid);
    });
}

void ThreadPool::workerThreadFunc() noexcept
{
    bool ret;
    while (!mQuit) {
        OsSignal::Recover();
        // try {
        function<void()> thread_func;
        ret = mTaskQueue.tryPop(thread_func);
        if (!ret) {
            break;
        }
        thread_func();
        thread_func = nullptr;
        // } catch (std::exception& e) {
        //     ERR("thread pool exception: {}\n", e.what());
        //     auto traceInfo = TraceInfo::GetAllTraceInfo();
        //     string traceStr = traceInfo->getBackTraceSymbols();
        //     ERR("{}\n", traceStr);
        // } catch (...) {
        //     ERR("state thread pool exception\n");
        // }
    }
}

void ThreadPool::clearThread()
{
    std::lock_guard<mutex> l(mMtx);
    --mCurrThreadSize;
}

void ThreadPool::stop()
{
    std::lock_guard<mutex> l(mMtx);
    mQuit = true;
    mTaskQueue.destroy();
    mDeadThreadQueue.destroy();
}

void ThreadPool::join()
{
    mDeadThread->join();
    mThreads.clear();
    mTaskQueue.clear();
}
