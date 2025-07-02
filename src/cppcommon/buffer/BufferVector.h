#pragma once

#include "tools/cpp_common.h"

class BufferVector {
public:
    void append(vector<char> buffer);
    int cut(char* buffer, int size);
private:
    std::mutex mMutex;
    vector<char> mBuffer;
};
