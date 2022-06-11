#include "MediaTimer.h"
#include "log/Log.h"

void MediaTimer::setSleepMs(long val)
{
    mSleepMs = val;
}

int MediaTimer::getSleepMs()
{
    return mSleepMs;
}

void MediaTimer::sleep()
{
    if (mSleepMs == 0) {
        next_30();
    } else {
        next();
    }
}

inline void MediaTimer::next()
{
    if (!mLastTimeStamp) {
        mLastTimeStamp = Tools::NowMs();
        return;
    }
    long now = Tools::NowMs();
    long last = mLastTimeStamp;
    long sleep_ms = mSleepMs;
    long real_sleep_ms = 0;
    real_sleep_ms = sleep_ms - (now - last);
    mLastTimeStamp += sleep_ms;
    if (real_sleep_ms <= 0) {
        WARN("media timer sleep not more than 0, real_sleep_ms:%ld sleep_ms:%ld now:%ld last:%ld\n", real_sleep_ms, sleep_ms, now, last);
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(real_sleep_ms));
}

inline void MediaTimer::next_30()
{
    if (!mLastTimeStamp) {
        mLastTimeStamp = Tools::NowMs();
        return;
    }
    long now = Tools::NowMs();
    long last = mLastTimeStamp;
    long sleep_ms = 0;
    long real_sleep_ms = 0;
    if (++mCount % 3 == 0) {
        mCount = 0;
        sleep_ms = 34;
    } else {
        sleep_ms = 33;
    }
    real_sleep_ms = sleep_ms - (now - last);
    mLastTimeStamp += sleep_ms;
    if (real_sleep_ms <= 0) {
        WARN("media timer sleep not more than 0, real_sleep_ms:%ld sleep_ms:%ld now:%ld last:%ld\n", real_sleep_ms, sleep_ms, now, last);
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(real_sleep_ms));
}
