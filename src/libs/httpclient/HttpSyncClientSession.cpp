#include "httpclient/HttpSyncClientSession.h"
#include "boost/beast/core/error.hpp"
#include "boost/beast/http/verb.hpp"
#include "boost/format.hpp"
#include "httpclient/HttpSyncClient.h"
#include "url/Url.h"
#include <memory>
#include <string>

HttpSyncClientSession::~HttpSyncClientSession()
{
    this->close();
}

void HttpSyncClientSession::setHost(string val)
{
    mHost = val;
}

void HttpSyncClientSession::setPort(int val)
{
    mPort = val;
}

int HttpSyncClientSession::connect()
{
    string port = std::to_string(mPort);
    tcp::resolver resolver(mIoc);
    boost::system::error_code ec;
    auto const results = resolver.resolve(mHost.c_str(), port.c_str(), ec);
    if (ec) {
        ERR("resolve error, host:{} port:{} msg:{}\n", mHost, port, ec.message());
        return -1;
    }
    mStream = std::make_unique<beast::tcp_stream>(mIoc);
    mStream->connect(results, ec);
    if (ec) {
        ERR("connect error, host:{} port:{} msg:{}\n", mHost, port, ec.message());
        return -1;
    }
    return 0;
}

void HttpSyncClientSession::close()
{
    if (!mStream) {
        return;
    }
    beast::error_code ec;
    mStream->socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec) {
        ERR("close error, host:{} port:{} msg:{}\n", mHost, mPort, ec.message());
    }
    mStream.reset();
}

int HttpSyncClientSession::httpGet(string path, vector<map<string, string>> headers, string& body)
{
    // Set up an HTTP GET request message
    http::request<http::string_body> req { http::verb::get, path, 11 };
    req.set(http::field::host, mHost);
    req.set(http::field::user_agent, HttpSyncClient::GetUserAgent());
    for (auto headerMap : headers) {
        for (auto val : headerMap) {
            req.set(val.first, val.second);
        }
    }

    // Send the HTTP request to the remote host
    http::write(*mStream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    http::read(*mStream, buffer, res);

    int code = res.result_int();
    string reason = res.reason();
    if (code / 100 != 2) {
        ERR("http fail, path:{} code:{} reason:{}\n", path, code, reason);
        return -1;
    }
    body = res.body();
    INFO("http success, path:{} code:{} reason:{} body:{}\n", path, code, reason, body);
    return 0;
}

int HttpSyncClientSession::httpPost(string path, vector<map<string, string>> headers, string contentType, string content, string& body)
{
    // Set up an HTTP POST request message
    http::request<http::string_body> req { http::verb::post, path, 11 };
    req.set(http::field::host, mHost);
    req.set(http::field::user_agent, HttpSyncClient::GetUserAgent());
    for (auto headerMap : headers) {
        for (auto val : headerMap) {
            req.set(val.first, val.second);
        }
    }
    if (!contentType.empty()) {
        req.set(http::field::content_type, contentType);
    }
    if (!content.empty()) {
        req.body() = content;
        req.prepare_payload();
    }

    // Send the HTTP request to the remote host
    http::write(*mStream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    http::read(*mStream, buffer, res);

    int code = res.result_int();
    string reason = res.reason();
    if (code / 100 != 2) {
        ERR("http fail, path:{} code:{} reason:{}\n", path, code, reason);
        return -1;
    }
    body = res.body();
    INFO("http success, path:{} content:{} code:{} reason:{} body:{}\n", path, content, code, reason, body);
    return 0;
}

int HttpSyncClientSession::httpMethod(string path, boost::beast::http::verb method, vector<map<string, string>> headers, string contentType, string content, string& body)
{
    // Set up an HTTP POST request message
    http::request<http::string_body> req { method, path, 11 };
    req.set(http::field::host, mHost);
    req.set(http::field::user_agent, HttpSyncClient::GetUserAgent());
    for (auto headerMap : headers) {
        for (auto val : headerMap) {
            req.set(val.first, val.second);
        }
    }
    if (!contentType.empty()) {
        req.set(http::field::content_type, contentType);
    }
    if (!content.empty()) {
        req.body() = content;
        req.prepare_payload();
    }

    // Send the HTTP request to the remote host
    http::write(*mStream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    http::read(*mStream, buffer, res);

    int code = res.result_int();
    string reason = res.reason();
    if (code / 100 != 2) {
        ERR("http fail, path:{} code:{} reason:{}\n", path, code, reason);
        return -1;
    }
    body = res.body();
    INFO("http success, path:{} content:{} code:{} reason:{} body:{}\n", path, content, code, reason, body);
    return 0;
}
