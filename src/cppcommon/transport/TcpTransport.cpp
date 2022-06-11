
#include "transport/TcpTransport.h"
#include "transport/TransportManager.h"

TcpTransport::TcpTransport(shared_ptr<boost::asio::io_context>& ioc)
{
    mIoc = ioc;
}

TcpTransport::~TcpTransport()
{
    boost::system::error_code ec;
    std::lock_guard<mutex> l(mMutex);
    if (mSock) {
        mSock->close(ec);
        mSock.reset();
    }
    if (mAcceptor) {
        mAcceptor->close(ec);
        mAcceptor.reset();
    }
    if (mIoc) {
        mIoc.reset();
    }
}

int TcpTransport::init(TransportTcpType type, string local_host, int local_port, string remote_host, int remote_port)
{
    if (TRANSPORT_TCP_TYPE_CLIENT == type) {
        mSock = std::make_shared<boost::asio::ip::tcp::socket>(*mIoc);
        if (local_port > 0) {
            boost::asio::ip::tcp::endpoint local_ep(boost::asio::ip::address::from_string(local_host), local_port);
            boost::system::error_code ec;
            mSock->bind(local_ep, ec);
            if (ec) {
                string err_msg = ec.message();
                ERR("bind %s:%d error: %s\n", local_host.c_str(), local_port, err_msg.c_str());
                return -1;
            }
        }
        do_connect(remote_host, remote_port);
    } else if (TRANSPORT_TCP_TYPE_SERVER == type) {
        mAcceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(*mIoc);
        boost::system::error_code ec;
        mAcceptor->open(boost::asio::ip::tcp::v4(), ec);
        if (ec) {
            string err_msg = ec.message();
            ERR("open %s:%d error: %s\n", local_host.c_str(), local_port, err_msg.c_str());
            return -1;
        }
        mAcceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) {
            string err_msg = ec.message();
            ERR("set_option %s:%d error: %s\n", local_host.c_str(), local_port, err_msg.c_str());
            return -1;
        }
        boost::asio::ip::tcp::endpoint local_ep(boost::asio::ip::address::from_string(local_host), local_port);
        mAcceptor->bind(local_ep, ec);
        if (ec) {
            string err_msg = ec.message();
            ERR("bind %s:%d error: %s\n", local_host.c_str(), local_port, err_msg.c_str());
            return -1;
        }
        mAcceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            string err_msg = ec.message();
            ERR("listen %s:%d error: %s\n", local_host.c_str(), local_port, err_msg.c_str());
            return -1;
        }
        do_accept();
    }
    mTransportTcpType = type;
    return 0;
}

void TcpTransport::setCb(std::function<void(TransportTcpEventType type, shared_ptr<TcpTransport> transport)> cb)
{
    mCb = cb;
}

void TcpTransport::do_connect(string host, int port)
{
    std::lock_guard<mutex> l(mMutex);
    boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::address::from_string(host), port);
    weak_ptr<TcpTransport> weak = shared_from_this();
    mSock->async_connect(remote_ep, [weak, this](boost::system::error_code ec) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_connect(ec);
    });
}

void TcpTransport::on_connect(boost::system::error_code ec)
{
    if (ec) {
        ERR("on_connect error: %s\n", ec.message().c_str());
        return;
    }
    if (mCb) {
        mCb(TRANSPORT_TCP_EVENT_TYPE_ON_CONNECT, shared_from_this());
    }
}

void TcpTransport::do_accept()
{
    std::lock_guard<mutex> l(mMutex);
    shared_ptr<boost::asio::ip::tcp::socket> new_sock = std::make_shared<boost::asio::ip::tcp::socket>(*mIoc);
    weak_ptr<TcpTransport> weak = shared_from_this();
    mAcceptor->async_accept(*new_sock, [weak, this, new_sock](boost::system::error_code ec) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_accept(new_sock, ec);
    });
}

void TcpTransport::on_accept(shared_ptr<boost::asio::ip::tcp::socket> new_sock, boost::system::error_code ec)
{
    if (ec) {
        ERR("on_accept error: %s\n", ec.message().c_str());
        return;
    }
    auto tcpTransport = std::make_shared<TcpTransport>(mIoc);
    tcpTransport->mTransportTcpType = TRANSPORT_TCP_TYPE_ACCEPT;
    tcpTransport->mSock = new_sock;
    if (mCb) {
        mCb(TRANSPORT_TCP_EVENT_TYPE_ON_ACCEPT, tcpTransport);
    }
    do_accept();
}

void TcpTransport::do_read_some(char* data, int size, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb)
{
    std::lock_guard<mutex> l(mMutex);
    mSock->async_receive(boost::asio::buffer(data, size), 0, cb);
}

void TcpTransport::do_read(char* data, int size, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb)
{
    std::lock_guard<mutex> l(mMutex);
    boost::asio::async_read(*mSock, boost::asio::buffer(data, size), cb);
}

void TcpTransport::do_write_some(char* data, int size, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb)
{
    std::lock_guard<mutex> l(mMutex);
    mSock->async_send(boost::asio::buffer(data, size), 0, cb);
}

void TcpTransport::on_write(char* data, int size, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb)
{
    std::lock_guard<mutex> l(mMutex);
    boost::asio::async_write(*mSock, boost::asio::buffer(data, size), cb);
}
