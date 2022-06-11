#pragma once

#include "tools/Tools.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <thread>

using std::condition_variable;
using std::function;
using std::list;
using std::map;
using std::mutex;
using std::string;

template <typename T>
class DelayedQueue {
public:
    DelayedQueue(int maxSize = 1000);
    ~DelayedQueue();
    bool push(T t);
    bool tryPush(T t);
    bool push(string taskId, long duration_ms, T t);
    bool tryPush(string taskId, long duration_ms, T t);
    void erase(string taskId);
    bool pop(T& t);
    bool tryPop(T& t);
    int size();
    int capacity();
    void clear(std::function<void(T t)> fn = nullptr);
    void destroy();

private:
    struct Task {
        string taskId;
        long ms;
        T t;
    };
    list<T> mQueue;
    list<Task> mDelayedQueue;
    map<string, typename list<Task>::iterator> mTaskMap;
    std::mutex mMtx;
    std::condition_variable mCvR;
    std::condition_variable mCvW;
    int mMaxSize;
    bool mQuit = false;
};

template <typename T>
DelayedQueue<T>::DelayedQueue(int maxSize)
{
    mMaxSize = maxSize;
}

template <typename T>
DelayedQueue<T>::~DelayedQueue()
{
    destroy();
}

template <typename T>
bool DelayedQueue<T>::push(T e)
{
    std::unique_lock<std::mutex> l(mMtx);
    mCvW.wait(l, [this] { return mQuit || mMaxSize <= 0 || mQueue.size() + mDelayedQueue.size() < mMaxSize; });
    if (mQuit) {
        return false;
    }
    mQueue.push_back(std::move(e));
    mCvR.notify_one();
    return true;
}

template <typename T>
bool DelayedQueue<T>::push(string taskId, long duration_ms, T e)
{
    std::unique_lock<std::mutex> l(mMtx);
    mCvW.wait(l, [this] { return mQuit || mMaxSize <= 0 || mQueue.size() + mDelayedQueue.size() < mMaxSize; });
    if (mQuit) {
        return false;
    }
    long ms = Tools::NowMs() + duration_ms;
    Task task = { taskId, ms, std::move(e) };
    auto it = std::upper_bound(mDelayedQueue.begin(), mDelayedQueue.end(), task, [](const Task& l, const Task& r) { return l.ms < r.ms; });
    it = mDelayedQueue.insert(it, std::move(task));
    mTaskMap.emplace(taskId, it);
    if (it == mDelayedQueue.begin()) {
        mCvR.notify_one();
    }
    return true;
}

template <typename T>
bool DelayedQueue<T>::tryPush(T e)
{
    std::unique_lock<std::mutex> l(mMtx);
    if (mQuit || (mMaxSize > 0 && mQueue.size() + mDelayedQueue.size() >= mMaxSize)) {
        return false;
    }
    mQueue.push_back(std::move(e));
    mCvR.notify_one();
    return true;
}

template <typename T>
bool DelayedQueue<T>::tryPush(string taskId, long duration_ms, T e)
{
    std::unique_lock<std::mutex> l(mMtx);
    if (mQuit || (mMaxSize > 0 && mQueue.size() + mDelayedQueue.size() >= mMaxSize)) {
        return false;
    }
    long ms = Tools::NowMs() + duration_ms;
    Task task = { taskId, ms, std::move(e) };
    auto it = std::upper_bound(mDelayedQueue.begin(), mDelayedQueue.end(), task, [](const Task& l, const Task& r) { return l.ms < r.ms; });
    it = mDelayedQueue.insert(it, std::move(task));
    mTaskMap.emplace(taskId, it);
    if (it == mDelayedQueue.begin()) {
        mCvR.notify_one();
    }
    return true;
}

template <typename T>
void DelayedQueue<T>::erase(string taskId)
{
    std::unique_lock<std::mutex> l(mMtx);
    auto it = mTaskMap.find(taskId);
    if (it == mTaskMap.end()) {
        return;
    }
    auto list_it = it->second;
    bool is_notify = (list_it == mDelayedQueue.begin());
    Task task = std::move(*list_it);
    mDelayedQueue.erase(list_it);
    mTaskMap.erase(it);
    if (is_notify) {
        mCvR.notify_one();
    }
    l.unlock();
}

template <typename T>
bool DelayedQueue<T>::pop(T& t)
{
    std::unique_lock<std::mutex> l(mMtx);
    while (true) {
        mCvR.wait(l, [this] { return mQuit || !mQueue.empty() || !mDelayedQueue.empty(); });
        if (mQueue.empty() && mDelayedQueue.empty()) {
            return false;
        }
        if (!mDelayedQueue.empty()) {
            Task& task = mDelayedQueue.front();
            long ms = task.ms;
            long now = Tools::NowMs();
            if (ms <= now) {
                t = std::move(task.t);
                mDelayedQueue.pop_front();
                mCvW.notify_one();
                return true;
            }
        }
        if (!mQueue.empty()) {
            t = std::move(mQueue.front());
            mQueue.pop_front();
            mCvW.notify_one();
            return true;
        }
        if (!mDelayedQueue.empty()) {
            Task& task = mDelayedQueue.front();
            long ms = task.ms;
            long now = Tools::NowMs();
            bool got = true;
            while (ms > now) {
                std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> abs_time((std::chrono::milliseconds(ms)));
                mCvR.wait_until(l, abs_time);
                if (!mQueue.empty() || mDelayedQueue.empty()) {
                    got = false;
                    break;
                }
                Task& task = mDelayedQueue.front();
                ms = task.ms;
                now = Tools::NowMs();
            }
            if (got) {
                t = std::move(task.t);
                mDelayedQueue.pop_front();
                mCvW.notify_one();
                return true;
            }
        }
    }
    return false;
}

template <typename T>
bool DelayedQueue<T>::tryPop(T& t)
{
    std::unique_lock<std::mutex> l(mMtx);
    if (!mDelayedQueue.empty()) {
        Task& task = mDelayedQueue.front();
        long ms = task.ms;
        long now = Tools::NowMs();
        if (ms <= now) {
            t = std::move(task.t);
            mDelayedQueue.pop_front();
            mCvW.notify_one();
            return true;
        }
    }
    if (!mQueue.empty()) {
        t = std::move(mQueue.front());
        mQueue.pop_front();
        mCvW.notify_one();
        return true;
    }
    return false;
}

template <typename T>
int DelayedQueue<T>::size()
{
    std::lock_guard<std::mutex> l(mMtx);
    return mQueue.size() + mDelayedQueue.size();
}

template <typename T>
int DelayedQueue<T>::capacity()
{
    std::lock_guard<std::mutex> l(mMtx);
    return mMaxSize;
}

template <typename T>
void DelayedQueue<T>::clear(std::function<void(T t)> fn)
{
    std::unique_lock<std::mutex> l(mMtx);
    for (auto it = mQueue.begin(); it != mQueue.end();) {
        T e = *it;
        it = mQueue.erase(it);
        if (fn) {
            fn(e);
        }
    }
    for (auto it = mDelayedQueue.begin(); it != mDelayedQueue.end();) {
        T e = std::move(it.t);
        it = mQueue.erase(it);
        if (fn) {
            fn(e);
        }
    }
    mCvW.notify_all();
}

template <typename T>
void DelayedQueue<T>::destroy()
{
    std::unique_lock<std::mutex> l(mMtx);
    mQuit = true;
    mCvR.notify_all();
    mCvW.notify_all();
}
