#pragma once
#include "threadpool/ThreadPoolBase.h"

using std::deque;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::thread;
using std::unique_ptr;
using std::vector;

class ThreadPoolPermanent : public ThreadPoolBase {
public:
    ThreadPoolPermanent(int maxSize, int queueMaxSize);
    ThreadPoolPermanent(int minSize, int maxSize, int queueMaxSize);
    ~ThreadPoolPermanent();
    int startThread(string stateId);
    void stopThread(string stateId);
    int execute(string stateId, function<void()> task);
};
