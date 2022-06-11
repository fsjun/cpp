#pragma once

#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>

using std::condition_variable;
using std::function;
using std::list;
using std::make_shared;
using std::mutex;
using std::shared_ptr;

template <typename T>
class ConnectionPool {
public:
    void setMinSize(int val)
    {
        mMinSize = val;
    }

    int getMinSize()
    {
        return mMinSize;
    }

    void setMaxSize(int val)
    {
        mMaxSize = val;
    }

    int getMaxSize()
    {
        return mMaxSize;
    }

    shared_ptr<T> getConnection(function<void(shared_ptr<T>)> cb)
    {
        std::unique_lock<mutex> l(mMutex);
        if (!mConnectionList.empty()) {
            auto conn = mConnectionList.front();
            mConnectionList.pop_front();
            return conn;
        }
        if (mCurrSize < mMaxSize) {
            ++mCurrSize;
            auto conn = make_shared<T>();
            if (cb) {
                cb(conn);
            }
            return conn;
        }
        mCv.wait(l);
        if (!mConnectionList.empty()) {
            auto conn = mConnectionList.front();
            mConnectionList.pop_front();
            return conn;
        }
        return nullptr;
    }

    void revokeConnection(shared_ptr<T> conn)
    {
        std::unique_lock<mutex> l(mMutex);
        if (mConnectionList.empty() || mCurrSize <= mMinSize) {
            mConnectionList.emplace_back(conn);
            mCv.notify_one();
            return;
        }
        --mCurrSize;
    }

protected:
    int mMinSize = 5;
    int mMaxSize = 5;
    int mCurrSize = 0;
    mutex mMutex;
    condition_variable mCv;
    list<shared_ptr<T>> mConnectionList;
};
