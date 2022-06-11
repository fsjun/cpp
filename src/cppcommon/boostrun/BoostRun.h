#pragma once

#include "boost/asio.hpp"
#include "threadpool/ThreadPool.h"
#include "tools/Singleton.h"
#include <iostream>
#include <memory>

using std::shared_ptr;
namespace asio = boost::asio;

class BoostRun : public Singleton<BoostRun>, public std::enable_shared_from_this<BoostRun> {
public:
    shared_ptr<asio::io_context> getIoContext();
    void run();
    void stop();

private:
    BoostRun();
    friend class Singleton<BoostRun>;

private:
    shared_ptr<asio::io_context> mIoContext;
};
