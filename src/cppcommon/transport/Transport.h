#pragma once

#include "log/Log.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"

class Transport {
public:
    Transport(shared_ptr<boost::asio::io_context> ioc = nullptr);
    virtual ~Transport();
    void setId(string val);

protected:
    mutex mMutex;
    shared_ptr<boost::asio::io_context> mIoContext;
    
    string mId;
    bool mIsWrite = false;
};
