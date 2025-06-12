#pragma once

#include "threadpool/BaseThread.h"
#include "threadpool/DelayedQueue.h"
#include "threadpool/Queue.h"
#include "threadpool/Thread.h"
#include "tools/cpp_common.h"
#include <memory>
#include <stop_token>
#include <thread>

using std::jthread;

class Jthread : public std::enable_shared_from_this<Jthread>, public BaseThread {
public:
    Jthread(int queueMaxSize = 1000);
    ~Jthread();
    int start();
    void stop();
    void join();
    bool postDelayedTask(string taskId, long ms, function<void()> func);
    void removeDelayedTask(string taskId);
    bool execute(function<void()> func);
    bool execute_block(function<void()> func);
    int size();

private:
    void threadFunc(std::stop_token st) noexcept;
    int process();

private:
    DelayedQueue<function<void()>> mTasks;
    unique_ptr<jthread> mThread;
};
