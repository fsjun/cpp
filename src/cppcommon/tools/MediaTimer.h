#pragma once

#include "tools/Tools.h"
#include <iostream>

class MediaTimer {
public:
    void setSleepMs(long val);
    int getSleepMs();
    void sleep();

private:
    inline void next();
    inline void next_30();

private:
    int mCount = 0;
    long mLastTimeStamp = 0;

    long mSleepMs = 0;
};
