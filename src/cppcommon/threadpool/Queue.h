#pragma once

#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

using std::condition_variable;
using std::function;
using std::list;
using std::mutex;
using std::string;

template <typename T>
class Queue {
public:
    Queue(int maxSize = 1000);
    ~Queue();
    bool push(T t);
    bool tryPush(T t);
    bool pop(T& t);
    bool tryPop(T& t);
    int size();
    int capacity();
    void clear(std::function<void(T t)> fn = nullptr);
    void destroy();

private:
    list<T> mQueue;
    std::mutex mMtx;
    std::condition_variable mCvR;
    std::condition_variable mCvW;
    int mMaxSize;
    bool mQuit = false;
};

template <typename T>
Queue<T>::Queue(int maxSize)
{
    mMaxSize = maxSize;
}

template <typename T>
Queue<T>::~Queue()
{
    destroy();
}

template <typename T>
bool Queue<T>::push(T e)
{
    std::unique_lock<std::mutex> l(mMtx);
    mCvW.wait(l, [this] { return mQuit || mMaxSize <= 0 || mQueue.size() < mMaxSize; });
    if (mQuit) {
        return false;
    }
    mQueue.push_back(std::move(e));
    mCvR.notify_one();
    return true;
}

template <typename T>
bool Queue<T>::tryPush(T e)
{
    std::unique_lock<std::mutex> l(mMtx);
    if (mQuit || (mMaxSize > 0 && mQueue.size() >= mMaxSize)) {
        return false;
    }
    mQueue.push_back(std::move(e));
    mCvR.notify_one();
    return true;
}

template <typename T>
bool Queue<T>::pop(T& t)
{
    std::unique_lock<std::mutex> l(mMtx);
    mCvR.wait(l, [this] { return mQuit || !mQueue.empty(); });
    if (mQueue.empty()) {
        return false;
    }
    t = std::move(mQueue.front());
    mQueue.pop_front();
    mCvW.notify_one();
    return true;
}

template <typename T>
bool Queue<T>::tryPop(T& t)
{
    std::unique_lock<std::mutex> l(mMtx);
    if (mQueue.empty()) {
        return false;
    }
    t = std::move(mQueue.front());
    mQueue.pop_front();
    mCvW.notify_one();
    return true;
}

template <typename T>
int Queue<T>::size()
{
    std::lock_guard<std::mutex> l(mMtx);
    return mQueue.size();
}

template <typename T>
int Queue<T>::capacity()
{
    std::lock_guard<std::mutex> l(mMtx);
    return mMaxSize;
}

template <typename T>
void Queue<T>::clear(std::function<void(T t)> fn)
{
    std::unique_lock<std::mutex> l(mMtx);
    for (auto it = mQueue.begin(); it != mQueue.end();) {
        T e = *it;
        it = mQueue.erase(it);
        if (fn) {
            fn(e);
        }
    }
    mCvW.notify_all();
}

template <typename T>
void Queue<T>::destroy()
{
    std::unique_lock<std::mutex> l(mMtx);
    mQuit = true;
    mCvR.notify_all();
    mCvW.notify_all();
}
