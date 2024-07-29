#include "WsServerSession.h"

WsServerSession::WsServerSession(tcp::socket&& socket, WebSocketServerCb cb)
    : mStream(std::move(socket))
{
    mCb = cb;
    auto remote_endpoint = mStream.next_layer().socket().remote_endpoint();
    mRemoteIp = remote_endpoint.address().to_string();
    mRemotePort = remote_endpoint.port();
}

// Get on the correct executor
void WsServerSession::start()
{
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    net::dispatch(mStream.get_executor(), beast::bind_front_handler(&WsServerSession::on_run, shared_from_this()));
}

// Start the asynchronous operation
void WsServerSession::on_run()
{
    // Set suggested timeout settings for the websocket
    mStream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    // Set a decorator to change the Server of the handshake
    mStream.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    }));
    // Accept the websocket handshake
    mStream.async_accept(beast::bind_front_handler(&WsServerSession::on_accept, shared_from_this()));
}

void WsServerSession::on_accept(beast::error_code ec)
{
    if (ec) {
        ERR("websocket accept error, remote {}:{} msg:{}\n", mRemoteIp, mRemotePort, ec.message());
        return;
    }
    mCb(shared_from_this(), WS_SERVER_CONNECT, nullptr, 0);
    // Read a message
    do_read();
}

void WsServerSession::do_read()
{
    // Clear the buffer
    mBuffer.consume(mBuffer.size());
    // Read a message into our buffer
    mStream.async_read(mBuffer, beast::bind_front_handler(&WsServerSession::on_read, shared_from_this()));
}

void WsServerSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    // This indicates that the session was closed
    if (ec == websocket::error::closed) {
        INFO("ws close by peer, remote {}:{}\n", mRemoteIp, mRemotePort);
        mCb(shared_from_this(), WS_SERVER_CLOSE, nullptr, 0);
        return;
    }
    if (ec) {
        ERR("ws read error, remote {}:{} msg:{}\n", mRemoteIp, mRemotePort, ec.message());
        mCb(shared_from_this(), WS_SERVER_ERROR, nullptr, 0);
        return;
    }
    auto data = mBuffer.data();
    WsServerEventType type = WS_SERVER_MESSAGE;
    bool isBinary = mStream.got_binary();
    if (isBinary) {
        type = WS_SERVER_BINARY;
        DEBUG("ws receive message, remote {}:{}\n", mRemoteIp, mRemotePort);
    } else {
        INFO("ws receive message, remote {}:{} message:{:.{}}\n", mRemoteIp, mRemotePort, (char*)data.data(), data.size());
    }
    mCb(shared_from_this(), type, (char*)data.data(), data.size());
    do_read();
}

int WsServerSession::do_write(string message, bool async)
{
    INFO("ws send message, remote {}:{} message:{}\n", mRemoteIp, mRemotePort, message);
    std::unique_lock<std::mutex> l(mMutex);
    if (async) {
        if (mIsSendError) {
            return -1;
        }
        shared_ptr<std::vector<char>> vec = make_shared<std::vector<char>>(message.begin(), message.end());
        if (isSend) {
            if (mQueue.size() >= mQueueSize) {
                mCv.wait(l, [this]() { return mQueue.size() < mQueueSize; });
            }
            if (isSend) {
                mQueue.emplace_back(std::make_pair(false, vec));
            } else {
                isSend = true;
                mStream.text(true);
                mStream.async_write(net::buffer(vec->data(), vec->size()), beast::bind_front_handler(&WsServerSession::on_write, shared_from_this(), vec));
            }
        } else {
            isSend = true;
            mStream.text(true);
            mStream.async_write(net::buffer(vec->data(), vec->size()), beast::bind_front_handler(&WsServerSession::on_write, shared_from_this(), vec));
        }
    } else {
        mStream.text(true);
        beast::error_code ec;
        mStream.write(net::buffer(message), ec);
        if (ec) {
            ERR("error ws write, remote {}:{} msg:{}\n", mRemoteIp, mRemotePort, ec.message());
            return -1;
        }
    }
    return 0;
}

int WsServerSession::do_write(char* data, int len, bool async)
{
    DEBUG("ws send data, remote {}:{}\n", mRemoteIp, mRemotePort);
    std::unique_lock<std::mutex> l(mMutex);
    if (async) {
        if (mIsSendError) {
            return -1;
        }
        // data maybe modify, when function return, need hold data.
        shared_ptr<std::vector<char>> vec = make_shared<std::vector<char>>(data, data + len);
        if (isSend) {
            if (mQueue.size() >= mQueueSize) {
                mCv.wait(l, [this]() { return mQueue.size() < mQueueSize; });
            }
            if (isSend) {
                mQueue.emplace_back(std::make_pair(true, vec));
            } else {
                isSend = true;
                mStream.binary(true);
                mStream.async_write(net::buffer(vec->data(), vec->size()), beast::bind_front_handler(&WsServerSession::on_write, shared_from_this(), vec));
            }
        } else {
            isSend = true;
            mStream.binary(true);
            mStream.async_write(net::buffer(vec->data(), vec->size()), beast::bind_front_handler(&WsServerSession::on_write, shared_from_this(), vec));
        }
    } else {
        mStream.binary(true);
        beast::error_code ec;
        mStream.write(net::buffer(data, len), ec);
        if (ec) {
            ERR("error ws write, remote {}:{} msg:{}\n", mRemoteIp, mRemotePort, ec.message());
            return -1;
        }
    }
    return 0;
}

void WsServerSession::on_write(shared_ptr<std::vector<char>> vec, beast::error_code ec, std::size_t bytes_transferred)
{
    std::lock_guard<std::mutex> l(mMutex);
    isSend = false;
    mCv.notify_one();
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        ERR("ws write error, remote {}:{} msg:{}\n", mRemoteIp, mRemotePort, ec.message());
        mIsSendError = true;
        return;
    }
    if (mQueue.empty()) {
        return;
    }
    auto msg = mQueue.front();
    mQueue.pop_front();
    auto item = msg.second;
    if (msg.first) {
        mStream.binary(true);
    } else {
        mStream.text(true);
    }
    isSend = true;
    mStream.async_write(net::buffer(item->data(), item->size()), beast::bind_front_handler(&WsServerSession::on_write, shared_from_this(), item));
}

string WsServerSession::getRemoteIp()
{
    return mRemoteIp;
}

int WsServerSession::getRemotePort()
{
    return mRemotePort;
}
