#include "transport/UdpTransport.h"

UdpTransport::UdpTransport(shared_ptr<boost::asio::io_context>& ioc)
{
    mIoContext = ioc;
}

UdpTransport::~UdpTransport()
{
    this->close();
    INFO("UdpTransport destructor, id:{}", mId);
}

int UdpTransport::init(string host, int port)
{
    mSock = std::make_shared<boost::asio::ip::udp::socket>(*mIoContext);
    boost::asio::ip::udp::endpoint ep(boost::asio::ip::address::from_string(host), port);
    boost::system::error_code ec;
    mSock->open(ep.protocol());
    mSock->bind(ep, ec);
    if (ec) {
        ERRLN("bind error address({}:{}) error:{} id:{}", host, port, ec.what(), mId);
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

void UdpTransport::setListener(weak_ptr<Listener> listener)
{
    mListener = listener;
}

void UdpTransport::read(shared_ptr<vector<char>> buff)
{
    std::lock_guard<mutex> l(mMutex);
    auto epPtr = std::make_shared<boost::asio::ip::udp::endpoint>();
    weak_ptr<UdpTransport> weak = shared_from_this();
    auto cb = [weak, epPtr, this, buff](const boost::system::error_code& err, const size_t& bytes) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_read(buff, *epPtr, err, bytes);
    };
    if (buff->empty()) {
        buff->resize(2048);
    }
    mSock->async_receive_from(boost::asio::buffer(buff->data(), buff->size()), *epPtr, cb);
}

void UdpTransport::write(shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint ep)
{
    std::lock_guard<mutex> l(mMutex);
    if (mIsWrite) {
        mPendingWrite.emplace_back(buff, ep);
        return;
    }
    mIsWrite = true;
    doWrite(buff, ep);
}

void UdpTransport::writePending()
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mPendingWrite.empty()) {
        mIsWrite = false;
        return;
    }
    auto it = mPendingWrite.front();
    mPendingWrite.pop_front();
    doWrite(it.first, it.second);
}

void UdpTransport::doWrite(shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint ep)
{
    weak_ptr<UdpTransport> weak = shared_from_this();
    auto cb = [weak, this, ep, buff](const boost::system::error_code& err, const size_t& bytes) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_write(buff, ep, err, bytes);
    };
    mSock->async_send_to(boost::asio::buffer(buff->data(), buff->size()), ep, cb);
}

void UdpTransport::on_read(shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint ep, const boost::system::error_code& err, const size_t& bytes)
{
    if (err) {
        ERRLN("on_read error:{} id:{}", err.what(), mId);
    }
    auto self = mListener.lock();
    if (!self) {
        return;
    }
    if (!err) {
        buff->resize(bytes);
    }
    self->onRead(shared_from_this(), buff, ep, err, bytes);
}

void UdpTransport::on_write(shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint ep, const boost::system::error_code& err, const size_t& bytes)
{
    if (err) {
        ERRLN("on_write error:{} id:{}", err.what(), mId);
    }
    writePending();
    auto self = mListener.lock();
    if (!self) {
        return;
    }
    self->onWrite(shared_from_this(), buff, ep, err, bytes);
}
