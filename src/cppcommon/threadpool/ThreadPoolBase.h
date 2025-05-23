#pragma once
#include "Thread.h"
#include <algorithm>
#include <deque>
#include <map>
#include <memory>
#include <vector>
#include "DelayedThread.h"

using std::deque;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::thread;
using std::unique_ptr;
using std::vector;

class ThreadPoolBase {
public:
    ThreadPoolBase(int minSize, int maxSize, int queueMaxSize);
    virtual ~ThreadPoolBase();
    int start();
    void stop();
    void join();

protected:
    shared_ptr<DelayedThread> startThread();
    void startThreadTimer(shared_ptr<DelayedThread> thread);
    void onThreadExpired(shared_ptr<DelayedThread> thread);

    void workerThreadFunc(string stateId) noexcept;
    bool clearThread(string stateId);

protected:
    long mOwner = 0;
    int mMinSize = 0;
    int mMaxSize = 0;
    int mQueueMaxSize = 0;

    mutex mMutex;
    std::condition_variable mCv;
    bool mRunning = false;
    string mTimerId = "threadPoolTimer";
    long mTime = 70000;
    long mExpireDuration = 60;
    list<shared_ptr<DelayedThread>> mIdleThreads;
    list<shared_ptr<DelayedThread>> mWorkerThreads;
};
