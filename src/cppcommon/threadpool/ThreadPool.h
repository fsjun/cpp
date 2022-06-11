#pragma once
#include "Queue.h"
#include "Thread.h"
#include <algorithm>
#include <map>
#include <vector>

using std::make_shared;
using std::map;
using std::shared_ptr;
using std::thread;
using std::vector;

class ThreadPool {
public:
    ThreadPool(int maxSize, int queueSize);
    ~ThreadPool();
    int execute(function<void()> task);
    void stop();
    void join();

private:
    void startThread(function<void()> thread_func);
    void clearThread();

    void workerThreadFunc() noexcept;

private:
    int mMaxThreadSize = 0;
    int mCurrThreadSize = 0;
    int mQueueSize = 0;
    bool mQuit = false;

    mutex mMtx;
    Queue<function<void()>> mTaskQueue;
    map<string, unique_ptr<Thread>> mThreads;
    Queue<string> mDeadThreadQueue;
    unique_ptr<Thread> mDeadThread;
};
