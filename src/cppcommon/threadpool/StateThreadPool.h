#pragma once
#include "QueueRef.h"
#include "Thread.h"
#include <algorithm>
#include <deque>
#include <map>
#include <memory>
#include <vector>

using std::deque;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::thread;
using std::unique_ptr;
using std::vector;

class StateThreadPool {
public:
    StateThreadPool(int maxSize, int queueSize);
    ~StateThreadPool();
    int execute(string stateId, function<void()> task);
    void stop();
    void join();

private:
    void workerThreadFunc(string stateId) noexcept;
    bool clearThread(string stateId);

private:
    long mOwner = 0;
    int mMaxThreadSize = 0;
    int mCurrThreadSize = 0;
    int mQueueSize = 0;
    bool mQuit = false;
    mutex mMtx;
    map<string, shared_ptr<QueueRef>> mTaskQueues;
    map<string, unique_ptr<Thread>> mThreads;
    deque<unique_ptr<Thread>> mDeadThreads;
    Queue<string> mDeadThreadQueue;
    unique_ptr<Thread> mDeadThread;
};
