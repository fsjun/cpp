#pragma once
#include "threadpool/ThreadPoolBase.h"

using std::deque;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::thread;
using std::unique_ptr;
using std::vector;

class PermanentThreadPool : public ThreadPoolBase {
public:
    PermanentThreadPool(int maxSize, int queueMaxSize);
    PermanentThreadPool(int minSize, int maxSize, int queueMaxSize);
    ~PermanentThreadPool();
    int startThread(string stateId);
    void stopThread(string stateId);
    int execute(string stateId, function<void()> task);
};
