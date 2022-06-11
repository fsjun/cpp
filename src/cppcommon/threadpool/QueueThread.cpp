#include "threadpool/QueueThread.h"
#include "threadpool/ThreadManager.h"
#include <memory>

QueueThread::QueueThread()
{
}

QueueThread::~QueueThread()
{
}

int QueueThread::start()
{
    mThread = std::make_unique<Thread>();
    mThread->start(std::bind(&QueueThread::thread_func, shared_from_this()));
    return 0;
}

void QueueThread::stop()
{
    mRuning = false;
    mTasks.destroy();
}

void QueueThread::join()
{
    mThread.reset();
}

bool QueueThread::postDelayedTask(string taskId, long ms, function<void()> func)
{
    return false;
}

void QueueThread::removeDelayedTask(string taskId)
{
}

bool QueueThread::execute(function<void()> func)
{
    return mTasks.push(func);
}

bool QueueThread::execute_block(function<void()> func)
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

void QueueThread::thread_func() noexcept
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

void QueueThread::clearThread()
{
    auto threadManager = ThreadManager::GetInstance();
    threadManager->push(shared_from_this());
}
