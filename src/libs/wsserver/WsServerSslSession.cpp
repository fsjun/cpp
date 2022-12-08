#include "WsServerSslSession.h"

WsServerSslSession::WsServerSslSession(tcp::socket&& socket, ssl::context& ctx, WebSocketServerCb cb)
    : mStream(std::move(socket), ctx)
{
    mCb = cb;
    auto remote_endpoint = mStream.next_layer().next_layer().socket().remote_endpoint();
    mRemoteIp = remote_endpoint.address().to_string();
    mRemotePort = remote_endpoint.port();
}

void WsServerSslSession::start()
{
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    net::dispatch(mStream.get_executor(), beast::bind_front_handler(&WsServerSslSession::on_run, shared_from_this()));
}

// Start the asynchronous operation
void WsServerSslSession::on_run()
{
    // Set the timeout.
    beast::get_lowest_layer(mStream).expires_after(std::chrono::seconds(10));
    // Perform the SSL handshake
    mStream.next_layer().async_handshake(ssl::stream_base::server, beast::bind_front_handler(&WsServerSslSession::on_handshake, shared_from_this()));
}

void WsServerSslSession::on_handshake(beast::error_code ec)
{
    if (ec) {
        ERR("wss handshake error, remote %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, ec.message().c_str());
        return;
    }
    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(mStream).expires_never();
    // Set suggested timeout settings for the websocket
    mStream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    // Set a decorator to change the Server of the handshake
    mStream.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    }));
    // Accept the websocket handshake
    mStream.async_accept(beast::bind_front_handler(&WsServerSslSession::on_accept, shared_from_this()));
}

void WsServerSslSession::on_accept(beast::error_code ec)
{
    if (ec) {
        ERR("wss accept error, remote %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, ec.message().c_str());
        return;
    }
    mCb(shared_from_this(), WS_SERVER_CONNECT, nullptr, 0);
    // Read a message
    do_read();
}

void WsServerSslSession::do_read()
{
    // Clear the buffer
    mBuffer.consume(mBuffer.size());
    // Read a message into our buffer
    mStream.async_read(mBuffer, beast::bind_front_handler(&WsServerSslSession::on_read, shared_from_this()));
}

void WsServerSslSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    // This indicates that the session was closed
    if (ec == websocket::error::closed) {
        INFO("wss close by peer, remote %s:%d\n", mRemoteIp.c_str(), mRemotePort);
        mCb(shared_from_this(), WS_SERVER_CLOSE, nullptr, 0);
        return;
    }
    if (ec) {
        ERR("wss read error, remote %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, ec.message().c_str());
        mCb(shared_from_this(), WS_SERVER_ERROR, nullptr, 0);
        return;
    }
    auto data = mBuffer.data();
    WsServerEventType type = WS_SERVER_MESSAGE;
    bool isBinary = mStream.got_binary();
    if (isBinary) {
        type = WS_SERVER_BINARY;
        DEBUG("wss receive message, remote %s:%d\n", mRemoteIp.c_str(), mRemotePort);
    } else {
        INFO("wss receive message, remote %s:%d message:%.*s\n", mRemoteIp.c_str(), mRemotePort, data.size(), (char*)data.data());
    }
    mCb(shared_from_this(), type, (char*)data.data(), data.size());
    do_read();
}

void WsServerSslSession::do_write(string message, bool async)
{
    INFO("wss send message, remote %s:%d message:%s\n", mRemoteIp.c_str(), mRemotePort, message.c_str());
    std::lock_guard<std::mutex> l(mMutex);
    mStream.text(true);
    if (async) {
        mStream.async_write(net::buffer(message), beast::bind_front_handler(&WsServerSslSession::on_write, shared_from_this()));
    } else {
        beast::error_code ec;
        mStream.write(net::buffer(message), ec);
        if (ec) {
            ERR("error ws write, remote %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, ec.message().c_str());
            return;
        }
    }
}

void WsServerSslSession::do_write(char* data, int len, bool async)
{
    DEBUG("wss send message, remote %s:%d\n", mRemoteIp.c_str(), mRemotePort);
    std::lock_guard<std::mutex> l(mMutex);
    if (async) {
        mStream.binary(true);
        mStream.async_write(net::buffer(data, len), beast::bind_front_handler(&WsServerSslSession::on_write, shared_from_this()));
    } else {
        beast::error_code ec;
        mStream.write(net::buffer(data, len), ec);
        if (ec) {
            ERR("error ws write, remote %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, ec.message().c_str());
            return;
        }
    }
}

void WsServerSslSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        ERR("wss write error, remote %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, ec.message().c_str());
        return;
    }
}

string WsServerSslSession::getRemoteIp()
{
    return mRemoteIp;
}

int WsServerSslSession::getRemotePort()
{
    return mRemotePort;
}
