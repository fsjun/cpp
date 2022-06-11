#include "QueueRef.h"

QueueRef::QueueRef(int queueSize)
    : Queue(queueSize)
{
}

void QueueRef::inc()
{
    ++mRefs;
}

unsigned int QueueRef::dec()
{
    return --mRefs;
}
