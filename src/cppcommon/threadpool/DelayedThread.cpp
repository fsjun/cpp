#include "threadpool/DelayedThread.h"
#include "threadpool/ThreadManager.h"
#include <memory>

DelayedThread::DelayedThread()
{
}

DelayedThread::~DelayedThread()
{
}

int DelayedThread::start()
{
    mThread = std::make_unique<Thread>();
    mThread->start(std::bind(&DelayedThread::thread_func, shared_from_this()));
    return 0;
}

void DelayedThread::stop()
{
    mRuning = false;
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

void DelayedThread::thread_func() noexcept
{
    bool ret;
    while (mRuning) {
        function<void()> func;
        ret = mTasks.pop(func);
        if (!ret) {
            break;
        }
        func();
    }
    clearThread();
}

void DelayedThread::clearThread()
{
    auto threadManager = ThreadManager::GetInstance();
    threadManager->push(shared_from_this());
}
