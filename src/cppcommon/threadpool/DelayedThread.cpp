#include "threadpool/DelayedThread.h"
#include "threadpool/ThreadManager.h"
#include <memory>

DelayedThread::DelayedThread(int queueMaxSize)
    : mTasks(queueMaxSize)
{
}

DelayedThread::~DelayedThread()
{
}

void DelayedThread::setId(string val)
{
    mId = val;
}

string DelayedThread::getId()
{
    return mId;
}

int DelayedThread::start()
{
    mThread = std::make_unique<Thread>();
    auto weak = weak_from_this();
    mThread->start([weak]() {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        self->thread_func();
    });
    return 0;
}

void DelayedThread::stop()
{
    mTasks.destroy();
}

void DelayedThread::join()
{
    mThread.reset();
}

bool DelayedThread::postDelayedTask(string taskId, long ms, function<void()> func)
{
    return mTasks.push(taskId, ms, func);
}

void DelayedThread::removeDelayedTask(string taskId)
{
    mTasks.erase(taskId);
}

bool DelayedThread::execute(function<void()> func)
{
    return mTasks.push(func);
}

bool DelayedThread::execute_block(function<void()> func)
{
    bool ret;
    Queue<int> queue;
    ret = execute([&queue, func, this]() {
        func();
        queue.push(0);
    });
    if (ret) {
        int t;
        queue.pop(t);
    }
    return ret;
}

bool DelayedThread::expire(int diff)
{
    if (mLastActiveTime == 0) {
        return false;
    }
    if (mTasks.size() > 0) {
        return false;
    }
    uint64_t now = Tools::Now();
    return now - mLastActiveTime > diff;
}

int DelayedThread::size()
{
    return mTasks.size();
}

void DelayedThread::thread_func() noexcept
{
    bool ret;
    while (mRunning) {
        function<void()> func;
        ret = mTasks.pop(func);
        if (!ret) {
            break;
        }
        func();
        mLastActiveTime = Tools::Now();
    }
    clearThread();
}

void DelayedThread::clearThread()
{
    auto threadManager = ThreadManager::GetInstance();
    threadManager->push(shared_from_this());
}
