#pragma once

#include "threadpool/ThreadPoolBase.h"

class StateThreadPool : public ThreadPoolBase {
public:
    StateThreadPool(int maxSize, int queueMaxSize);
    StateThreadPool(int minSize, int maxSize, int queueMaxSize);
    ~StateThreadPool();
    int execute(string stateId, function<void()> task);
};
