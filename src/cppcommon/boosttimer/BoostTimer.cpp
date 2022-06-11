#include "BoostTimer.h"
#include "boostuuid/Uuid.h"

int BoostTimer::init(shared_ptr<boost::asio::io_context> ioContext, shared_ptr<ThreadPool> threadPool)
{
    if (ioContext) {
        mIoContext = ioContext;
    } else {
        mIoContext = make_shared<boost::asio::io_context>();
    }
    mThreadPool = threadPool;
    return 0;
}

int BoostTimer::start()
{
    auto ioc = mIoContext;
    mThreadPool->execute([ioc]() {
        boost::asio::io_context::work work(*ioc);
        ioc->run();
    });
    return 0;
}

string BoostTimer::startTimer(string timerId, int seconds, function<void()> cb)
{
    std::lock_guard<mutex> l(mMutex);
    shared_ptr<boost::asio::deadline_timer> timer;
    auto it = mTimers.find(timerId);
    if (it == mTimers.end()) {
        timer = make_shared<boost::asio::deadline_timer>(*mIoContext);
        mTimers.emplace(timerId, timer);
    } else {
        timer = it->second;
    }
    timer->expires_from_now(boost::posix_time::seconds(seconds));
    weak_ptr<BoostTimer> weak = shared_from_this();
    auto timerCb = [timerId, weak, cb](const boost::system::error_code& ec) {
        if (ec) {
            ERR("timer error, timerId:%s err:%s\n", timerId.c_str(), ec.message().c_str());
            return;
        }
        auto self = weak.lock();
        if (!self) {
            ERR("boost timer has destroyed\n");
            return;
        }
        // self->stopTimer(timerId);
        cb();
    };
    timer->async_wait(timerCb);
    return timerId;
}

void BoostTimer::stopTimer(string timerId)
{
    std::lock_guard<mutex> l(mMutex);
    auto it = mTimers.find(timerId);
    if (it == mTimers.end()) {
        return;
    }
    auto timer = it->second;
    timer->cancel();
    mTimers.erase(it);
}

void BoostTimer::stopAllTimer()
{
    std::unique_lock<mutex> l(mMutex);
    for (auto it : mTimers) {
        it.second->cancel();
    }
    l.unlock();
    mIoContext->stop();
}

void BoostTimer::delTimer(string timerId)
{
    std::lock_guard<mutex> l(mMutex);
    mTimers.erase(timerId);
}
