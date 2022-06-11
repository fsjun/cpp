#include "transport/UdpTransport.h"

UdpTransport::UdpTransport(shared_ptr<boost::asio::io_context>& ioc)
{
    mIoContext = ioc;
}

UdpTransport::~UdpTransport()
{
    this->close();
}

int UdpTransport::init(string host, int port)
{
    mSock = std::make_shared<boost::asio::ip::udp::socket>(*mIoContext);
    boost::asio::ip::udp::endpoint ep(boost::asio::ip::address::from_string(host), port);
    boost::system::error_code ec;
    mSock->open(ep.protocol());
    mSock->bind(ep, ec);
    if (ec) {
        ERR("bind error address(%s:%d): %s\n", host.c_str(), port, ec.message().c_str());
        return -1;
    }
    auto local_ep = mSock->local_endpoint();
    mLocalHost = local_ep.address().to_string();
    mLocalPort = local_ep.port();
    return 0;
}

void UdpTransport::close()
{
    std::lock_guard<mutex> l(mMutex);
    if (mSock) {
        boost::system::error_code ec;
        mSock->close(ec);
        mSock = nullptr;
    }
}

string UdpTransport::getLocalHost()
{
    return mLocalHost;
}

int UdpTransport::getLocalPort()
{
    return mLocalPort;
}

void UdpTransport::do_read(char* data, int size, boost::asio::ip::udp::endpoint& ep, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb)
{
    std::lock_guard<mutex> l(mMutex);
    mSock->async_receive_from(boost::asio::buffer(data, size), ep, cb);
}

void UdpTransport::do_write(char* data, int size, boost::asio::ip::udp::endpoint& ep, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb)
{
    std::lock_guard<mutex> l(mMutex);
    mSock->async_send_to(boost::asio::buffer(data, size), ep, cb);
}
