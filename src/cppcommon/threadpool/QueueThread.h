#pragma once

#include "threadpool/BaseThread.h"
#include "threadpool/Queue.h"
#include "threadpool/Thread.h"
#include "tools/cpp_common.h"
#include <memory>

class QueueThread : public std::enable_shared_from_this<QueueThread>, public BaseThread {
public:
    QueueThread();
    ~QueueThread();
    int start();
    void stop();
    void join();
    bool postDelayedTask(string taskId, long ms, function<void()> func);
    void removeDelayedTask(string taskId);
    bool execute(function<void()> func);
    bool execute_block(function<void()> func);

private:
    void thread_func() noexcept;
    void clearThread();

private:
    bool mRuning = true;
    Queue<function<void()>> mTasks;
    unique_ptr<Thread> mThread;
};
