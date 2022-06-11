#pragma once

#include "threadpool/Queue.h"
#include "tools/cpp_common.h"

class BaseThread {
public:
    virtual ~BaseThread() = default;
    virtual int start() = 0;
    virtual void stop() = 0;
    virtual void join() = 0;
    virtual bool postDelayedTask(string taskId, long ms, function<void()> func) = 0;
    virtual void removeDelayedTask(string taskId) = 0;
    virtual bool execute(function<void()> func) = 0;
    virtual bool execute_block(function<void()> func) = 0;
    template <typename T>
    bool execute_block(T& t_out, function<T()> func)
    {
        bool ret;
        Queue<T> queue;
        ret = execute([&queue, func, this]() {
            T t = func();
            queue.push(t);
        });
        if (ret) {
            queue.pop(t_out);
        }
        return ret;
    }
};
