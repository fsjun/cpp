#include "StateThreadPool.h"
#include "osinfo/OsSignal.h"
#include "osinfo/ThreadInfo.h"
#include "osinfo/TraceInfo.h"

StateThreadPool::StateThreadPool(int maxSize, int queueSize)
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

StateThreadPool::~StateThreadPool()
{
}

int StateThreadPool::execute(string stateId, function<void()> task)
{
    std::unique_lock<mutex> l(mMtx);
    mOwner = ThreadInfo::GetTid();
    if (mQuit) {
        mOwner = 0;
        return -1;
    }
    auto it = mTaskQueues.find(stateId);
    if (it == mTaskQueues.end()) {
        if (mMaxThreadSize > 0 && mCurrThreadSize >= mMaxThreadSize) {
            mOwner = 0;
            ERR("state thread pool exceed max, current:%d max %d\n", mCurrThreadSize, mMaxThreadSize);
            return -1;
        }
        auto iter = mThreads.find(stateId);
        if (iter != mThreads.end()) {
            mOwner = 0;
            ERR("task queue not exist, but thread is exist, stateId:%s\n", stateId.c_str());
            return -1;
        }
        ++mCurrThreadSize;
        auto queue = make_shared<QueueRef>(mQueueSize);
        mTaskQueues.emplace(stateId, queue);
        auto ret = mThreads.emplace(stateId, unique_ptr<Thread>(new Thread()));
        auto& thread = ret.first->second;
        queue->inc();
        thread->start(std::bind(&StateThreadPool::workerThreadFunc, this, stateId));
        mOwner = 0;
        l.unlock();
        queue->push(task);
        return 0;
    }
    auto queue = it->second;
    queue->inc();
    mOwner = 0;
    l.unlock();
    queue->push(task);
    return 0;
}

bool StateThreadPool::clearThread(string stateId)
{
    std::unique_lock<mutex> l(mMtx);
    mOwner = ThreadInfo::GetTid();
    if (mQuit) {
        mOwner = 0;
        return true;
    }
    auto it = mTaskQueues.find(stateId);
    if (it == mTaskQueues.end()) {
        mOwner = 0;
        return true;
    }
    auto queue = it->second;
    unsigned int refs = queue->dec();
    if (refs <= 0) {
        --mCurrThreadSize;
        mDeadThreads.push_back(std::move(mThreads[stateId]));
        mThreads.erase(stateId);
        mTaskQueues.erase(stateId);
        mDeadThreadQueue.push(stateId);
        mOwner = 0;
        l.unlock();
        queue.reset();
        return true;
    }
    mOwner = 0;
    l.unlock();
    queue.reset();
    return false;
}

void StateThreadPool::workerThreadFunc(string stateId) noexcept
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
            break;
        }
        thread_func();
        thread_func = nullptr;
        ret = clearThread(stateId);
        if (ret) {
            break;
        }
        // } catch (std::exception& e) {
        //     ERR("state thread pool exception: %s\n", e.what());
        //     auto traceInfo = TraceInfo::GetAllTraceInfo();
        //     string traceStr = traceInfo->getBackTraceSymbols();
        //     ERR("%s\n", traceStr.c_str());
        // } catch (...) {
        //     ERR("state thread pool exception\n");
        // }
    }
}

void StateThreadPool::stop()
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

void StateThreadPool::join()
{
    mDeadThread->join();
    mDeadThreads.clear();
    mThreads.clear();
    mTaskQueues.clear();
}
