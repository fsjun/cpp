#pragma once

#include "threadpool/BaseThread.h"
#include "threadpool/DelayedQueue.h"
#include "threadpool/Queue.h"
#include "threadpool/Thread.h"
#include "tools/cpp_common.h"
#include <memory>

class DelayedThread : public std::enable_shared_from_this<DelayedThread>, public BaseThread {
public:
    DelayedThread(int queueMaxSize = 1000);
    ~DelayedThread();
    void setId(string val);
    string getId();
    int start();
    void stop();
    void join();
    bool postDelayedTask(string taskId, long ms, function<void()> func);
    void removeDelayedTask(string taskId);
    bool execute(function<void()> func);
    bool execute_block(function<void()> func);
    bool expire(int diff = 60);
    int size();

private:
    void thread_func() noexcept;
    void clearThread();

private:
    bool mRunning = true;
    string mId;
    DelayedQueue<function<void()>> mTasks;
    unique_ptr<Thread> mThread;
    uint64_t mLastActiveTime = 0;
};
