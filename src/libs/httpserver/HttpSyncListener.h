#pragma once

#include "HttpServer.h"
#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

class HttpSyncListener : public std::enable_shared_from_this<HttpSyncListener> {
public:
    HttpSyncListener(boost::asio::io_context& ioc, ssl::context& ctx, bool sslEnable, tcp::endpoint endpoint, shared_ptr<HttpServer>& server);
    void start();
    void run();

private:
    bool mIsRun = true;
    bool ssl_enable_;
    ssl::context ctx_;
    tcp::acceptor acceptor_;
    std::shared_ptr<HttpServer> mServer;
};
