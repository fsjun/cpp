#pragma once

#include "HttpClient.h"
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
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

class HttpClientSslSession : public std::enable_shared_from_this<HttpClientSslSession> {
public:
    HttpClientSslSession(shared_ptr<net::io_context> ioContext, ssl::context& ctx, HttpClientCb cb);
    void httpGet(string host, int port, string target, string query);
    void httpPost(string host, int port, string target, string query, string contentType, string body);

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_shutdown(beast::error_code ec);
    void do_close();

private:
    shared_ptr<net::io_context> mIoContext;
    HttpClientCb mCb;
    string mUrl;
    tcp::resolver mResolver;
    beast::ssl_stream<beast::tcp_stream> mStream;
    beast::flat_buffer mBuffer;
    http::request<http::string_body> mReq;
    http::response<http::string_body> mRes;
    string mLocalIp;
    int mLocalPort = 0;
};
