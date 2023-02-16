#pragma once

#include "boost/asio.hpp"
#include "boost/asio/io_context.hpp"
#include "tools/cpp_common.h"

class WatchDog {
public:
    void addProgram(string path);
    void run();

private:
    shared_ptr<boost::asio::io_context> mIoContext = make_shared<boost::asio::io_context>();
    int mIntervalSecond = 5;
};
