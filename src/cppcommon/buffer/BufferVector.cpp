#include "buffer/BufferVector.h"
#include <mutex>

void BufferVector::append(vector<char> buffer)
{
    std::lock_guard l(mMutex);
    mBuffer.insert(mBuffer.end(), buffer.begin(), buffer.end());
}

int BufferVector::cut(char* buffer, int size)
{
    std::lock_guard l(mMutex);
    if (size > mBuffer.size()) {
        return -1;
    }
    memcpy(buffer, mBuffer.data(), size);
    mBuffer.erase(mBuffer.begin(), mBuffer.begin() + size);
    return 0;
}
