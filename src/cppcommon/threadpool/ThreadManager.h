#pragma once

#include "threadpool/BaseThread.h"
#include "threadpool/Queue.h"
#include "threadpool/Thread.h"
#include "tools/Singleton.h"
#include <memory>

class ThreadManager : public Singleton<ThreadManager>, public std::enable_shared_from_this<ThreadManager> {
public:
    int start();
    int stop();
    void join();
    bool push(shared_ptr<BaseThread> val);

private:
    void thread_func() noexcept;

private:
    bool mRuning = true;
    unique_ptr<Thread> mThread;
    Queue<shared_ptr<BaseThread>> mDeadThreadQueue;
};
