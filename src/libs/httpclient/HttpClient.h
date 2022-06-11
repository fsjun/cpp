#pragma once

#include "log/Log.h"
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <functional>
#include <iostream>
#include <memory>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using std::function;
using std::shared_ptr;
using std::string;

typedef function<void(bool success, int httpCode, string httpReason, string body)> HttpClientCb;

class HttpClient : public std::enable_shared_from_this<HttpClient> {
public:
    HttpClient(shared_ptr<net::io_context> ioContext);
    int httpGet(string url, HttpClientCb cb);
    int httpPost(string url, string contentType, string body, HttpClientCb cb);
    static string GetUserAgent();

private:
    void load_root_certificates(ssl::context& ctx);

private:
    shared_ptr<net::io_context> mIoContext;
};
