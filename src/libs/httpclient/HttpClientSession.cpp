#include "HttpClientSession.h"
#include <boost/format.hpp>
#include <sstream>

using std::ostringstream;

HttpClientSession::HttpClientSession(shared_ptr<net::io_context> ioContext, HttpClientCb cb)
    : mResolver(net::make_strand(*ioContext))
    , mStream(net::make_strand(*ioContext))
{
    mIoContext = ioContext;
    mCb = cb;
}

void HttpClientSession::httpGet(string host, int port, string target, string query)
{
    if (target.empty()) {
        target = "/";
    }
    // Set up an HTTP GET request message
    mReq.version(11);
    mReq.method(http::verb::get);
    mReq.target(boost::str(boost::format("%s%s") % target % query));
    mReq.set(http::field::host, host);
    mReq.set(http::field::user_agent, HttpClient::GetUserAgent());
    mReq.set(http::field::accept, "*/*");
    mUrl = boost::str(boost::format("http://%s:%d%s%s") % host % port % target % query);
    INFOLN("http get, url:{}\n", mUrl);
    // Look up the domain name
    mResolver.async_resolve(host, std::to_string(port), beast::bind_front_handler(&HttpClientSession::on_resolve, shared_from_this()));
}

void HttpClientSession::httpPost(string host, int port, string target, string query, string contentType, string body)
{
    if (target.empty()) {
        target = "/";
    }
    // Set up an HTTP POST request message
    mReq.version(11);
    mReq.method(http::verb::post);
    mReq.target(boost::str(boost::format("%s%s") % target % query));
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
    mUrl = boost::str(boost::format("http://%s:%d%s%s") % host % port % target % query);
    INFOLN("http post, url:{} contentType:{} body:{}\n", mUrl, contentType, body);
    // Look up the domain name
    mResolver.async_resolve(host, std::to_string(port), beast::bind_front_handler(&HttpClientSession::on_resolve, shared_from_this()));
}

void HttpClientSession::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) {
        ERRLN("http resolve error, url:{} msg:{}\n", mUrl, ec.message());
        mCb(false, 500, "resolve", "");
        return;
    }

    // Set a timeout on the operation
    mStream.expires_after(std::chrono::seconds(10));

    // Make the connection on the IP address we get from a lookup
    mStream.async_connect(results, beast::bind_front_handler(&HttpClientSession::on_connect, shared_from_this()));
}

void HttpClientSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
{
    if (ec) {
        ERRLN("http connect error, url:{} msg:{}\n", mUrl, ec.message());
        mCb(false, 500, "connect", "");
        return;
    }
    auto local_endpoint = mStream.socket().local_endpoint();
    mLocalIp = local_endpoint.address().to_string();
    mLocalPort = local_endpoint.port();
    // Send the HTTP request to the remote host
    http::async_write(mStream, mReq, beast::bind_front_handler(&HttpClientSession::on_write, shared_from_this()));
}

void HttpClientSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        ERRLN("http write error, url:{} local {}:{} msg:{}\n", mUrl, mLocalIp, mLocalPort, ec.message());
        mCb(false, 500, "write", "");
        return;
    }
    // Receive the HTTP response
    http::async_read(mStream, mBuffer, mRes, beast::bind_front_handler(&HttpClientSession::on_read, shared_from_this()));
}

void HttpClientSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        // This means they closed the connection
        if (ec == http::error::end_of_stream) {
            do_close();
        }
        ERRLN("http read error, url:{} local {}:{} msg:{}\n", mUrl, mLocalIp, mLocalPort, ec.message());
        mCb(false, 500, "read", "");
        return;
    }
    int code = mRes.result_int();
    string reason = mRes.reason();
    string body = mRes.body();
    INFOLN("http success, url:{} code:{} reason:{} body:{} local {}:{}\n", mUrl, code, reason, body, mLocalIp, mLocalPort);
    mCb(true, code, reason, body);
    // Gracefully close the socket
    mStream.socket().shutdown(tcp::socket::shutdown_both, ec);
    // not_connected happens sometimes so don't bother reporting it.
    if (ec && ec != beast::errc::not_connected) {
        ERRLN("http shutdown error, url:{} local {}:{} msg:{}\n", mUrl, mLocalIp, mLocalPort, ec.message());
        return;
    }
    // If we get here then the connection is closed gracefully
}

void HttpClientSession::do_close()
{
    // Send a TCP shutdown
    beast::error_code ec;
    mStream.socket().shutdown(tcp::socket::shutdown_send, ec);
    // At this point the connection is closed gracefully
}
