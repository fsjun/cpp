#include "threadpool/Jthread.h"
#include <memory>

Jthread::Jthread(int queueMaxSize)
    : mTasks(queueMaxSize)
{
}

Jthread::~Jthread()
{
}

int Jthread::start()
{
    auto weak = weak_from_this();
    mThread = std::make_unique<jthread>([weak](std::stop_token st) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        self->threadFunc(st);
    });
    return 0;
}

void Jthread::stop()
{
    mThread->request_stop();
    mTasks.destroy();
}

void Jthread::join()
{
    mThread.reset();
}

bool Jthread::postDelayedTask(string taskId, long ms, function<void()> func)
{
    return mTasks.push(taskId, ms, func);
}

void Jthread::removeDelayedTask(string taskId)
{
    mTasks.erase(taskId);
}

bool Jthread::execute(function<void()> func)
{
    return mTasks.push(func);
}

bool Jthread::execute_block(function<void()> func)
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

int Jthread::size()
{
    return mTasks.size();
}

void Jthread::threadFunc(std::stop_token st) noexcept
{
    while (!st.stop_requested()) {
        process();
    }
}

int Jthread::process()
{
    function<void()> func;
    bool ret = mTasks.pop(func);
    if (!ret) {
        return -1;
    }
    func();
    return 0;
}
