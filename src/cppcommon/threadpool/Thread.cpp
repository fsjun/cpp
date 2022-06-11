#include "Thread.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <exception>
#include <iostream>

using std::exception;

Thread::~Thread()
{
    join();
}

void Thread::start(function<void()> thread_func)
{
    mThread.reset(new thread(thread_func));
}

void Thread::join()
{
    if (mThread && mThread->joinable()) {
        mThread->join();
        mThread.reset();
    }
}

string Thread::GenUUID()
{
    boost::uuids::random_generator rgen;
    boost::uuids::uuid ranUUID = rgen();
    std::ostringstream oss;
    oss << ranUUID;
    return oss.str();
}
