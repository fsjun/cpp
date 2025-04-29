#pragma once

#include "threadpool/ThreadPoolBase.h"

class ThreadPoolState : public ThreadPoolBase {
public:
    ThreadPoolState(int maxSize, int queueMaxSize);
    ThreadPoolState(int minSize, int maxSize, int queueMaxSize);
    ~ThreadPoolState();
    int execute(string stateId, function<void()> task);
};
