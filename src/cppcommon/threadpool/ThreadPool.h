#pragma once
#include "DelayedThread.h"
#include <algorithm>
#include <condition_variable>
#include <map>
#include <vector>
#include "threadpool/ThreadPoolBase.h"

using std::make_shared;
using std::map;
using std::shared_ptr;
using std::thread;
using std::vector;

class ThreadPool : public ThreadPoolBase {
public:
    ThreadPool(int maxSize, int queueMaxSize);
    ThreadPool(int minSize, int maxSize, int queueMaxSize);
    ~ThreadPool();
    virtual int execute(function<void()> task);
};
