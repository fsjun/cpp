#include "HttpClientSslSession.h"
#include "boost/format.hpp"

HttpClientSslSession::HttpClientSslSession(shared_ptr<net::io_context> ioContext, ssl::context& ctx,
    HttpClientCb cb)
    : mResolver(net::make_strand(*ioContext))
    , mStream(net::make_strand(*ioContext), ctx)
{
    mIoContext = ioContext;
    mCb = cb;
}

void HttpClientSslSession::httpGet(string host, int port, string target, string query)
{
    if (target.empty()) {
        target = "/";
    }
    mUrl = boost::str(boost::format("https://%s:%d%s%s") % host % port % target % query);
    INFOLN("https get, url:{}", mUrl);
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(mStream.native_handle(), host.c_str())) {
        beast::error_code ec { static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
        ERRLN("set tlsext host name error, msg:{}", ec.message());
        mCb(false, 500, "set host name", "");
        return;
    }
    // Set up an HTTP GET request message
    mReq.version(11);
    mReq.method(http::verb::get);
    mReq.target(target);
    mReq.set(http::field::host, host);
    mReq.set(http::field::user_agent, HttpClient::GetUserAgent());
    mReq.set(http::field::accept, "*/*");
    // Look up the domain name
    mResolver.async_resolve(host, std::to_string(port), beast::bind_front_handler(&HttpClientSslSession::on_resolve, shared_from_this()));
}

void HttpClientSslSession::httpPost(string host, int port, string target, string query, string contentType, string body)
{
    if (target.empty()) {
        target = "/";
    }
    mUrl = boost::str(boost::format("https://%s:%d%s%s") % host % port % target % query);
    INFOLN("https post, url:{} contentType:{} body:{}", mUrl, contentType, body);
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(mStream.native_handle(), host.c_str())) {
        beast::error_code ec { static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
        ERRLN("set tlsext host name error, msg:{}", ec.message());
        mCb(false, 500, "set host name", "");
        return;
    }
    // Set up an HTTP GET request message
    mReq.version(11);
    mReq.method(http::verb::post);
    mReq.target(target);
    mReq.set(http::field::host, host);
    mReq.set(http::field::user_agent, HttpClient::GetUserAgent());
    mReq.set(http::field::accept, "*/*");
    if (!contentType.empty()) {
        mReq.set(http::field::content_type, contentType);
    }
    if (!body.empty()) {
        mReq.body() = body;
        mReq.prepare_payload();
    }
    // Look up the domain name
    mResolver.async_resolve(host, std::to_string(port), beast::bind_front_handler(&HttpClientSslSession::on_resolve, shared_from_this()));
}

void HttpClientSslSession::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) {
        ERRLN("https resolve error, url:{} msg:{}", mUrl, ec.message());
        mCb(false, 500, "resolve", "");
        return;
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(mStream).expires_after(std::chrono::seconds(10));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(mStream).async_connect(results, beast::bind_front_handler(&HttpClientSslSession::on_connect, shared_from_this()));
}

void HttpClientSslSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
{
    if (ec) {
        ERRLN("https connect error, url:{} msg:{}", mUrl, ec.message());
        mCb(false, 500, "connect", "");
        return;
    }
    auto local_endpoint = mStream.next_layer().socket().local_endpoint();
    mLocalIp = local_endpoint.address().to_string();
    mLocalPort = local_endpoint.port();
    // Perform the SSL handshake
    mStream.async_handshake(ssl::stream_base::client, beast::bind_front_handler(&HttpClientSslSession::on_handshake, shared_from_this()));
}

void HttpClientSslSession::on_handshake(beast::error_code ec)
{
    if (ec) {
        ERRLN("https handshake error, url:{} local {}:{} msg:{}", mUrl, mLocalIp, mLocalPort, ec.message());
        mCb(false, 500, "handshake", "");
        return;
    }
    // Send the HTTP request to the remote host
    http::async_write(mStream, mReq, beast::bind_front_handler(&HttpClientSslSession::on_write, shared_from_this()));
}

void HttpClientSslSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        ERRLN("https write error, url:{} local {}:{} msg:{}", mUrl, mLocalIp, mLocalPort, ec.message());
        mCb(false, 500, "write", "");
        return;
    }
    // Receive the HTTP response
    http::async_read(mStream, mBuffer, mRes, beast::bind_front_handler(&HttpClientSslSession::on_read, shared_from_this()));
}

void HttpClientSslSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        // This means they closed the connection
        if (ec == http::error::end_of_stream) {
            do_close();
        }
        ERRLN("https read error, url:{} local {}:{} msg:{}", mUrl, mLocalIp, mLocalPort, ec.message());
        mCb(false, 500, "read", "");
        return;
    }
    int code = mRes.result_int();
    string reason = mRes.reason();
    string body = mRes.body();
    INFOLN("https get success, url:{} code:{} reason:{} body:{} local {}:{}", mUrl, code, reason, body, mLocalIp, mLocalPort);
    mCb(true, code, reason, body);
    // Set a timeout on the operation
    beast::get_lowest_layer(mStream).expires_after(std::chrono::seconds(30));
    // Gracefully close the stream
    mStream.async_shutdown(beast::bind_front_handler(&HttpClientSslSession::on_shutdown, shared_from_this()));
}

void HttpClientSslSession::on_shutdown(beast::error_code ec)
{
    if (ec == net::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }
    if (ec) {
        ERRLN("https shutdown error, url:{} local {}:{} msg:{}", mUrl, mLocalIp, mLocalPort, ec.message());
        return;
    }
    // If we get here then the connection is closed gracefully
}

void HttpClientSslSession::do_close()
{
    // Set the timeout.
    beast::get_lowest_layer(mStream).expires_after(std::chrono::seconds(30));
    // Perform the SSL shutdown
    mStream.async_shutdown(beast::bind_front_handler(&HttpClientSslSession::on_shutdown, shared_from_this()));
}
