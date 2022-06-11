#pragma once

#include "threadpool/ThreadPool.h"
#include "tools/Singleton.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"

class BoostTimer : public Singleton<BoostTimer>, public std::enable_shared_from_this<BoostTimer> {
public:
    int init(shared_ptr<boost::asio::io_context> ioContext, shared_ptr<ThreadPool> threadPool);
    int start();
    string startTimer(string timerId, int seconds, function<void()> cb);
    void stopTimer(string timerId);
    void stopAllTimer();
    void delTimer(string timerId);

private:
    std::mutex mMutex;
    map<string, shared_ptr<boost::asio::deadline_timer>> mTimers;
    shared_ptr<boost::asio::io_context> mIoContext;
    shared_ptr<ThreadPool> mThreadPool;
};
