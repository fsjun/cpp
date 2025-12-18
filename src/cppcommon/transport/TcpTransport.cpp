
#include "transport/TcpTransport.h"

TcpTransport::TcpTransport(shared_ptr<boost::asio::io_context>& ioc) : Transport(ioc)
{
}

TcpTransport::~TcpTransport()
{
    close();
    INFOLN("TcpTransport destructor id:{}", mId);
}

int TcpTransport::init(bool isClient, string local_host, int local_port, string remote_host, int remote_port)
{
    if (isClient) {
        mIsWrite = true;
        mSock = std::make_shared<boost::asio::ip::tcp::socket>(*mIoContext);
        if (local_port > 0) {
            boost::asio::ip::tcp::endpoint local_ep(boost::asio::ip::address::from_string(local_host), local_port);
            boost::system::error_code ec;
            mSock->bind(local_ep, ec);
            if (ec) {
                string err_msg = ec.what();
                ERRLN("bind {}:{} error:{} id:{}", local_host, local_port, err_msg, mId);
                return -1;
            }
            auto le = mSock->local_endpoint();
            mLocalPort = le.port();
            INFOLN("local_ep is {}:{} id:{}", le.address().to_string(), le.port(), mId);
        }
        do_connect(remote_host, remote_port);
    } else {
        mAcceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(*mIoContext);
        boost::system::error_code ec;
        mAcceptor->open(boost::asio::ip::tcp::v4(), ec);
        if (ec) {
            string err_msg = ec.what();
            ERRLN("open {}:{} error:{} id:{}", local_host, local_port, err_msg, mId);
            return -1;
        }
        mAcceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) {
            string err_msg = ec.what();
            ERRLN("set_option {}:{} error:{} id:{}", local_host, local_port, err_msg, mId);
            return -1;
        }
        boost::asio::ip::tcp::endpoint local_ep(boost::asio::ip::address::from_string(local_host), local_port);
        mAcceptor->bind(local_ep, ec);
        if (ec) {
            string err_msg = ec.what();
            ERRLN("bind {}:{} error:{} id:{}", local_host, local_port, err_msg, mId);
            return -1;
        }
        auto le = mAcceptor->local_endpoint();
        mLocalPort = le.port();
        INFOLN("local_ep is {}:{} id:{}", le.address().to_string(), le.port(), mId);
        mAcceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            string err_msg = ec.what();
            ERRLN("listen {}:{} error:{} id:{}", local_host, local_port, err_msg, mId);
            return -1;
        }
        do_accept();
    }
    return 0;
}

int TcpTransport::getLocalPort()
{
    return mLocalPort;
}

void TcpTransport::setListener(weak_ptr<Listener> listener)
{
    mListener = listener;
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
        ERRLN("on_connect fail, error:{} id:{}", ec.what(), mId);
    } else {
        INFOLN("on_connect success, error:{} id:{}", ec.what(), mId);
    }
    writePending();
    auto self = mListener.lock();
    if (!self) {
        return;
    }
    self->onConnect(shared_from_this(), ec);
}

void TcpTransport::do_accept()
{
    std::lock_guard<mutex> l(mMutex);
    shared_ptr<boost::asio::ip::tcp::socket> new_sock = std::make_shared<boost::asio::ip::tcp::socket>(*mIoContext);
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
        ERRLN("on_accept error:{} id:{}", ec.what(), mId);
    }
    auto self = mListener.lock();
    if (!self) {
        return;
    }
    if (ec) {
        self->onAccept(shared_from_this(), ec);
    } else {
        auto tcpTransport = std::make_shared<TcpTransport>(mIoContext);
        tcpTransport->mSock = new_sock;
        self->onAccept(tcpTransport, ec);
    }
}

void TcpTransport::do_read_some(shared_ptr<vector<char>> buff)
{
    std::lock_guard<mutex> l(mMutex);
    weak_ptr<TcpTransport> weak = shared_from_this();
    auto cb = [weak, this, buff](const boost::system::error_code& err, const size_t& bytes) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_read(buff, err, bytes);
    };
    if (buff->empty()) {
        buff->resize(2048);
    }
    mSock->async_receive(boost::asio::buffer(buff->data(), buff->size()), 0, cb);
}

void TcpTransport::do_read(shared_ptr<vector<char>> buff)
{
    std::lock_guard<mutex> l(mMutex);
    weak_ptr<TcpTransport> weak = shared_from_this();
    auto cb = [weak, this, buff](const boost::system::error_code& err, const size_t& bytes) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_read(buff, err, bytes);
    };
    if (buff->empty()) {
        buff->resize(2048);
    }
    boost::asio::async_read(*mSock, boost::asio::buffer(buff->data(), buff->size()), cb);
}

void TcpTransport::async_read_until(shared_ptr<vector<char>> buff, char delimiter)
{
    std::lock_guard<mutex> l(mMutex);
    weak_ptr<TcpTransport> weak = shared_from_this();
    auto cb = [weak, this, buff, delimiter](const boost::system::error_code& err, const size_t& bytes) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_read_until(buff, delimiter, err, bytes);
    };
    boost::asio::async_read_until(*mSock, mStreambuffer, delimiter, cb);
}

void TcpTransport::do_write_some(shared_ptr<vector<char>> buff)
{
    std::lock_guard<mutex> l(mMutex);
    weak_ptr<TcpTransport> weak = shared_from_this();
    auto cb = [weak, this, buff](const boost::system::error_code& err, const size_t& bytes) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_write(buff, err, bytes);
    };
    mSock->async_send(boost::asio::buffer(buff->data(), buff->size()), 0, cb);
}

void TcpTransport::write(shared_ptr<vector<char>> buff)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mIsWrite) {
        mPendingWrite.emplace_back(buff);
        return;
    }
    mIsWrite = true;
    doWrite(buff);
}

void TcpTransport::close()
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
    if (mIoContext) {
        mIoContext.reset();
    } 
}

void TcpTransport::writePending()
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mPendingWrite.empty()) {
        mIsWrite = false;
        return;
    }
    auto buff = mPendingWrite.front();
    mPendingWrite.pop_front();
    doWrite(buff);
}

void TcpTransport::doWrite(shared_ptr<vector<char>> buff)
{
    weak_ptr<TcpTransport> weak = shared_from_this();
    auto cb = [weak, this, buff](const boost::system::error_code& err, const size_t& bytes) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        on_write(buff, err, bytes);
    };
    boost::asio::async_write(*mSock, boost::asio::buffer(buff->data(), buff->size()), cb);
}

void TcpTransport::on_read(shared_ptr<vector<char>> buff, const boost::system::error_code& err, const size_t& bytes)
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
    self->onRead(shared_from_this(), buff, err, bytes);
}

void TcpTransport::on_read_until(shared_ptr<vector<char>> buff, char delimiter, const boost::system::error_code& err, const size_t& bytes)
{
    if (err) {
        ERRLN("on_read_until error:{} id:{}", err.what(), mId);
    }
    auto self = mListener.lock();
    if (!self) {
        return;
    }
    if (!err) {
        std::istream is(&mStreambuffer);
        std::string str;
        std::getline(is, str, delimiter);
        buff->assign(str.begin(), str.end());
        if (!is.eof()) {
            buff->emplace_back(delimiter);
        }
    }
    self->onRead(shared_from_this(), buff, err, bytes);
}

void TcpTransport::on_write(shared_ptr<vector<char>> buff, const boost::system::error_code& err, const size_t& bytes)
{
    if (err) {
        ERRLN("on_write error:{} id:{}", err.what(), mId);
    }
    writePending();
    auto self = mListener.lock();
    if (!self) {
        return;
    }
    self->onWrite(shared_from_this(), buff, err, bytes);
}