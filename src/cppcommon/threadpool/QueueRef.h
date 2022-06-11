#pragma once

#include "Queue.h"

class QueueRef : public Queue<function<void()>> {
public:
    QueueRef(int queueSize);
    void inc();
    unsigned int dec();

private:
    unsigned int mRefs = 0;
};