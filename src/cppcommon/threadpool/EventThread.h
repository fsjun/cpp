#pragma once

#include "threadpool/Thread.h"
#include "threadpool/Queue.h"
#include "threadpool/Event.h"
#include "threadpool/BaseThread.h"
#include "threadpool/ThreadManager.h"
#include "log/Log.h"
#include <memory>

template <class T>
class EventThread : public std::enable_shared_from_this<EventThread<T>>, public BaseThread {
public:
    int start() {
        mThread = std::make_unique<Thread>();
        mThread->start(std::bind(&EventThread::thread_func, this->shared_from_this()));
        return 0;
    }

    void stop() {
        mRuning = false;
        mQueue.destroy();
    }

    void join() {
        mThread.reset();
    }

    bool push(Event<T> e) {
        return mQueue.push(e);
    }

    virtual bool postDelayedTask(string taskId, long ms, function<void()> func) { return true; }
    virtual void removeDelayedTask(string taskId){}
    virtual bool execute(function<void()> func) { return true; }
    virtual bool execute_block(function<void()> func) { return true; }

    void setHandle(map<string, function<void(T &)>>& handler) {
        std::lock_guard l(mMutex);
        mHandler = handler;
    }

private:
    void thread_func() noexcept {
        bool ret;
        while (mRuning) {
            Event<T> e;
            ret = mQueue.pop(e);
            if (!ret) {
                break;
            }
            cout << e.id << endl;
            auto it = mHandler.find(e.id);
            if (it == mHandler.end()) {
                WARN("miss event handler id:{}\n", e.id);
                continue;
            }
            auto func = it->second;
            func(e.t);
        }
        clearThread();
    }

    void clearThread()
    {
        auto threadManager = ThreadManager::GetInstance();
        threadManager->push(this->shared_from_this());
    }

private:
    bool mRuning = true;
    Queue<Event<T>> mQueue;
    unique_ptr<Thread> mThread;
    std::mutex mMutex;
    map<string, function<void(T &)>> mHandler;
};
