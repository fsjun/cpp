#pragma once

#include <cstring>

class Buffer {
public:
    Buffer(int size);
    ~Buffer();
    bool isEmpty();
    bool isFull();
    int getUsedSize();
    int capacity();
    int cut(char* buf, int len);
    int copy(char* buf, int len);
    int appendCover(char* buf, int len);
    int append(char* buf, int len);
    int fill(int val, int len);
    int extract(char* delimiter, int delimiterLen, char*& data, int& len);

private:
    char* findDelimiter(char* delimiter, int delimiterLen);
    bool matchDelimiter(char* p, char* delimiter, int delimiterLen);

private:
    char* mBuffer;
    int mBufferLen;
    char* mBufferStart;
    char* mBufferEnd;
};
