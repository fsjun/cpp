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

void WsClientSslSession::do_write(string message, bool async)
{
    INFOLN("wss write, remote {}:{} local {}:{} message:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, message);
    mStream.text(true);
    if (async) {
        // Send the message
        mStream.async_write(net::buffer(message), beast::bind_front_handler(&WsClientSslSession::on_write, shared_from_this()));
    } else {
        beast::error_code ec;
        mStream.write(net::buffer(message), ec);
        if (ec) {
            ERRLN("error ws write, remote {}:{} local {}:{}, msg:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, ec.message());
            return;
        }
    }
}

void WsClientSslSession::do_write(char* data, int len, bool async)
{
    DEBUGLN("ws write, remote {}:{} local {}:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort);
    mStream.binary(true);
    if (async) {
        // Send the message
        mStream.async_write(net::buffer(data, len), beast::bind_front_handler(&WsClientSslSession::on_write, shared_from_this()));
    } else {
        beast::error_code ec;
        mStream.write(net::buffer(data, len), ec);
        if (ec) {
            ERRLN("error ws write, remote{}s:{} local {}:{}, msg:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, ec.message());
            return;
        }
    }
}

void WsClientSslSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        ERRLN("error wss write, remote {}:{} local {}:{}, msg:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, ec.message());
        return;
    }
}

void WsClientSslSession::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) {
        ERRLN("error wss resolve, remote {}:{}, msg:{}\n", mHost, mRemotePort, mLocalIp, mLocalPort, ec.message());
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
        ERRLN("error ws connect, remote {}:{}, msg:{}\n", mRemoteIp, mRemotePort, ec.message());
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
        ERRLN("error wss ssl handshake, remote {}:{} local {}:{} msg:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, ec.message());
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
        ERRLN("error wss handshake, remote {}:{} local {}:{} msg:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, ec.message());
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
        INFOLN("close wss, remote {}:{} local {}:{} msg:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, ec.message());
        mCb(shared_from_this(), WS_CLIENT_CLOSE, nullptr, 0);
        return;
    }
    if (ec) {
        ERRLN("error wss read, remote {}:{} local {}:{} msg:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, ec.message());
        mCb(shared_from_this(), WS_CLIENT_ERROR, nullptr, 0);
        return;
    }
    auto data = mBuffer.data();
    WsClientEventType type = WS_CLIENT_MESSAGE;
    bool isBinary = mStream.got_binary();
    if (isBinary) {
        type = WS_CLIENT_BINARY;
        DEBUGLN("wss receive message, remote {}:{}\n", mRemoteIp, mRemotePort);
    } else {
        INFOLN("wss receive message, remote {}:{} message:{:.{}}\n", mRemoteIp, mRemotePort, (char*)data.data(), data.size());
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
        ERRLN("error ws close, remote {}:{} local {}:{} msg:{}\n", mRemoteIp, mRemotePort, mLocalIp, mLocalPort, ec.message());
        return;
    }
}
