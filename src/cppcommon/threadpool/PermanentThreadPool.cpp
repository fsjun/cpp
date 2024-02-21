#include "PermanentThreadPool.h"
#include "osinfo/OsSignal.h"
#include "osinfo/ThreadInfo.h"
#include "osinfo/TraceInfo.h"

PermanentThreadPool::PermanentThreadPool(int maxSize, int queueSize)
    : mDeadThreadQueue(maxSize)
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
            mOwner = ThreadInfo::GetTid();
            if (mQuit) {
                mOwner = 0;
                break;
            }
            auto thread = std::move(mDeadThreads.front());
            mDeadThreads.pop_front();
            mOwner = 0;
            l.unlock();
            thread.reset();
        }
    });
}

PermanentThreadPool::~PermanentThreadPool()
{
}

int PermanentThreadPool::execute(string stateId, function<void()> task)
{
    std::unique_lock<mutex> l(mMtx);
    mOwner = ThreadInfo::GetTid();
    if (mQuit) {
        mOwner = 0;
        return -1;
    }
    auto it = mTaskQueues.find(stateId);
    if (it == mTaskQueues.end()) {
        ERR("permanent thread is not exist when execute, stateId:{}\n", stateId.c_str());
        mOwner = 0;
        return -1;
    }
    auto queue = it->second;
    mOwner = 0;
    l.unlock();
    queue->push(task);
    return 0;
}

int PermanentThreadPool::startThread(string stateId)
{
    std::unique_lock<mutex> l(mMtx);
    mOwner = ThreadInfo::GetTid();
    if (mQuit) {
        mOwner = 0;
        return -1;
    }
    auto it = mTaskQueues.find(stateId);
    if (it != mTaskQueues.end()) {
        mOwner = 0;
        WARN("permanent thread is exist when start thread, stateId:{}\n", stateId.c_str());
        return 0;
    }
    if (mMaxThreadSize > 0 && mCurrThreadSize >= mMaxThreadSize) {
        mOwner = 0;
        ERR("permanent thread pool exceed max, current:{} max {}\n", mCurrThreadSize, mMaxThreadSize);
        return -1;
    }
    auto iter = mThreads.find(stateId);
    if (iter != mThreads.end()) {
        mOwner = 0;
        ERR("task queue not exist, but thread is exist, stateId:{}\n", stateId.c_str());
        return -1;
    }
    ++mCurrThreadSize;
    auto queue = make_shared<Queue<function<void()>>>(mQueueSize);
    mTaskQueues.emplace(stateId, queue);
    auto ret = mThreads.emplace(stateId, unique_ptr<Thread>(new Thread()));
    auto& thread = ret.first->second;
    thread->start(std::bind(&PermanentThreadPool::workerThreadFunc, this, stateId));
    mOwner = 0;
    return 0;
}

void PermanentThreadPool::stopThread(string stateId)
{
    std::unique_lock<mutex> l(mMtx);
    mOwner = ThreadInfo::GetTid();
    if (mQuit) {
        mOwner = 0;
        return;
    }
    auto it = mTaskQueues.find(stateId);
    if (it == mTaskQueues.end()) {
        mOwner = 0;
        WARN("permanent thread is not exist when stop thread, stateId:{}\n", stateId.c_str());
        return;
    }
    auto queue = it->second;
    mOwner = 0;
    l.unlock();
    queue->destroy();
}

bool PermanentThreadPool::clearThread(string stateId)
{
    std::unique_lock<mutex> l(mMtx);
    mOwner = ThreadInfo::GetTid();
    if (mQuit) {
        mOwner = 0;
        return true;
    }
    --mCurrThreadSize;
    mDeadThreads.push_back(std::move(mThreads[stateId]));
    mThreads.erase(stateId);
    shared_ptr<Queue<function<void()>>> queue;
    auto it = mTaskQueues.find(stateId);
    if (it != mTaskQueues.end()) {
        queue = it->second;
    }
    mTaskQueues.erase(stateId);
    mDeadThreadQueue.push(stateId);
    mOwner = 0;
    l.unlock();
    queue.reset();
    return true;
}

void PermanentThreadPool::workerThreadFunc(string stateId) noexcept
{
    bool ret;
    std::unique_lock<mutex> l(mMtx);
    mOwner = ThreadInfo::GetTid();
    auto it = mTaskQueues.find(stateId);
    if (it == mTaskQueues.end()) {
        mOwner = 0;
        return;
    }
    auto queue = it->second;
    mOwner = 0;
    l.unlock();
    while (!mQuit) {
        OsSignal::Recover();
        // try {
        function<void()> thread_func;
        ret = queue->pop(thread_func);
        if (!ret) {
            clearThread(stateId);
            break;
        }
        thread_func();
        thread_func = nullptr;
        // } catch (std::exception& e) {
        //     ERR("permanent thread pool exception: {}\n", e.what());
        //     auto traceInfo = TraceInfo::GetAllTraceInfo();
        //     string traceStr = traceInfo->getBackTraceSymbols();
        //     ERR("{}\n", traceStr);
        // } catch (...) {
        //     ERR("permanent thread pool exception\n");
        // }
    }
}

void PermanentThreadPool::stop()
{
    std::unique_lock<mutex> l(mMtx);
    mOwner = ThreadInfo::GetTid();
    mQuit = true;
    for (auto it : mTaskQueues) {
        auto queue = it.second;
        queue->destroy();
    }
    mDeadThreadQueue.destroy();
    mOwner = 0;
}

void PermanentThreadPool::join()
{
    mDeadThread->join();
    mDeadThreads.clear();
    mThreads.clear();
    mTaskQueues.clear();
}
