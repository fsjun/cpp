#pragma once

#include "HttpServer.h"
#include <algorithm>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/ssl.hpp>

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

// Handles an HTTP server connection
class SslSession : public std::enable_shared_from_this<SslSession> {
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda {
        SslSession& self_;

        explicit send_lambda(SslSession& self)
            : self_(self)
        {
        }

        template <bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            self_.res_ = sp;

            // Write the response
            http::async_write(
                self_.stream_,
                *sp,
                std::bind(
                    &SslSession::on_write,
                    self_.shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2,
                    sp->need_eof()));
        }
    };

    bool ssl_enable_;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;
    shared_ptr<HttpServer> mServer;
    string mRemoteIp;
    int mRemotePort = 0;
    shared_ptr<boost::asio::io_context> mIoContext;

public:
    // Take ownership of the socket
    explicit SslSession(shared_ptr<boost::asio::io_context> ioc, shared_ptr<tcp::socket> socket, ssl::context& ctx, shared_ptr<HttpServer>& server);
    // Start the asynchronous operation
    void start();
    void on_handshake(boost::system::error_code ec);
    void do_read();
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
    void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close);
    void do_close();
    void on_shutdown(boost::system::error_code ec);
};
