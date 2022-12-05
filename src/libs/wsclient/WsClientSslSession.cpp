#include "WsClientSslSession.h"
#include <boost/format.hpp>

WsClientSslSession::WsClientSslSession(shared_ptr<net::io_context> ioContext, ssl::context& ctx, string host, int port, string path, string query, WebSocketClientCb cb)
    : mResolver(net::make_strand(*ioContext))
    , mStream(net::make_strand(*ioContext), ctx)
{
    mHost = host;
    mRemotePort = port;
    mPath = path;
    mQuery = query;
    mCb = cb;
}

void WsClientSslSession::start()
{
    // Look up the domain name
    mResolver.async_resolve(mHost, std::to_string(mRemotePort), beast::bind_front_handler(&WsClientSslSession::on_resolve, shared_from_this()));
}

void WsClientSslSession::do_write(string message)
{
    INFO("wss write, remote %s:%d local %s:%d message:%s\n", mRemoteIp.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort, message.c_str());
    mStream.text(true);
    // Send the message
    mStream.async_write(net::buffer(message), beast::bind_front_handler(&WsClientSslSession::on_write, shared_from_this()));
}

void WsClientSslSession::do_write(char* data, int len)
{
    DEBUG("ws write, remote %s:%d local %s:%d\n", mRemoteIp.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort);
    mStream.binary(true);
    // Send the message
    mStream.async_write(net::buffer(data, len), beast::bind_front_handler(&WsClientSslSession::on_write, shared_from_this()));
}

void WsClientSslSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        ERR("error wss write, remote %s:%d local %s:%d, msg:%s\n", mRemoteIp.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort, ec.message().c_str());
        return;
    }
}

void WsClientSslSession::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) {
        ERR("error wss resolve, remote %s:%d, msg:%s\n", mHost.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort, ec.message().c_str());
        mCb(shared_from_this(), WS_CLIENT_ERROR, nullptr, 0);
        return;
    }
    mRemoteIp = results->endpoint().address().to_string();
    // Set a timeout on the operation
    beast::get_lowest_layer(mStream).expires_after(std::chrono::seconds(10));
    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(mStream).async_connect(results, beast::bind_front_handler(&WsClientSslSession::on_connect, shared_from_this()));
}

void WsClientSslSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type type)
{
    if (ec) {
        ERR("error ws connect, remote %s:%d, msg:%s\n", mRemoteIp.c_str(), mRemotePort, ec.message().c_str());
        mCb(shared_from_this(), WS_CLIENT_ERROR, nullptr, 0);
        return;
    }
    auto local_endpoint = mStream.next_layer().next_layer().socket().local_endpoint();
    mLocalIp = local_endpoint.address().to_string();
    mLocalPort = local_endpoint.port();
    // Set a timeout on the operation
    beast::get_lowest_layer(mStream).expires_after(std::chrono::seconds(10));
    // Perform the SSL handshake
    mStream.next_layer().async_handshake(ssl::stream_base::client, beast::bind_front_handler(&WsClientSslSession::on_ssl_handshake, shared_from_this()));
}

void WsClientSslSession::on_ssl_handshake(beast::error_code ec)
{
    if (ec) {
        ERR("error wss ssl handshake, remote %s:%d local %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort, ec.message().c_str());
        mCb(shared_from_this(), WS_CLIENT_ERROR, nullptr, 0);
        return;
    }
    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(mStream).expires_never();
    // Set suggested timeout settings for the websocket
    mStream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    // Set a decorator to change the User-Agent of the handshake
    mStream.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
        req.set(http::field::user_agent, WebSocketClient::GetUserAgent());
    }));
    string target = boost::str(boost::format("%s%s") % mPath % mQuery);
    if (target.empty()) {
        target = "/";
    }
    // Perform the websocket handshake
    mStream.async_handshake(mHost, target, beast::bind_front_handler(&WsClientSslSession::on_handshake, shared_from_this()));
}

void WsClientSslSession::on_handshake(beast::error_code ec)
{
    if (ec) {
        ERR("error wss handshake, remote %s:%d local %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort, ec.message().c_str());
        mCb(shared_from_this(), WS_CLIENT_ERROR, nullptr, 0);
        return;
    }
    mCb(shared_from_this(), WS_CLIENT_CONNECT, nullptr, 0);
    do_read();
}

void WsClientSslSession::do_read()
{
    mBuffer.consume(mBuffer.size());
    // Read a message into our buffer
    mStream.async_read(mBuffer, beast::bind_front_handler(&WsClientSslSession::on_read, shared_from_this()));
}

void WsClientSslSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    // This indicates that the session was closed
    if (ec == websocket::error::closed) {
        INFO("close wss, remote %s:%d local %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort, ec.message().c_str());
        mCb(shared_from_this(), WS_CLIENT_CLOSE, nullptr, 0);
        return;
    }
    if (ec) {
        ERR("error wss read, remote %s:%d local %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort, ec.message().c_str());
        return;
    }
    auto data = mBuffer.data();
    WsClientEventType type = WS_CLIENT_MESSAGE;
    bool isBinary = mStream.got_binary();
    if (isBinary) {
        type = WS_CLIENT_BINARY;
        DEBUG("wss receive message, remote %s:%d\n", mRemoteIp.c_str(), mRemotePort);
    } else {
        INFO("wss receive message, remote %s:%d message:%.*s\n", mRemoteIp.c_str(), mRemotePort, data.size(), (char*)data.data());
    }
    mCb(shared_from_this(), type, (char*)data.data(), data.size());
    do_read();
}

void WsClientSslSession::do_close()
{
    // Close the WebSocket connection
    mStream.async_close(websocket::close_code::normal, beast::bind_front_handler(&WsClientSslSession::on_close, shared_from_this()));
}

void WsClientSslSession::on_close(beast::error_code ec)
{
    if (ec) {
        ERR("error ws close, remote %s:%d local %s:%d msg:%s\n", mRemoteIp.c_str(), mRemotePort, mLocalIp.c_str(), mLocalPort, ec.message().c_str());
        return;
    }
}
