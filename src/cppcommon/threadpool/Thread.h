#pragma once

#include <functional>
#include <memory>
#include <sstream>
#include <thread>

using std::function;
using std::shared_ptr;
using std::string;
using std::thread;
using std::unique_ptr;

class Thread : public std::enable_shared_from_this<Thread> {
public:
    ~Thread();
    void start(function<void()> thread_func);
    void join();

    static string GenUUID();

private:
    unique_ptr<thread> mThread;
};
