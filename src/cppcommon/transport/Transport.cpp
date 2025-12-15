#include "transport/Transport.h"
#include "log/Log.h"

Transport::Transport(shared_ptr<boost::asio::io_context> ioc)
{
    if (nullptr == ioc) {
        ioc = make_shared<boost::asio::io_context>();
    }
    mIoContext = ioc;
}

Transport::~Transport()
{
    INFOLN("transport destructor, id:{}", mId);
}

void Transport::setId(string val)
{
    mId = val;
}
