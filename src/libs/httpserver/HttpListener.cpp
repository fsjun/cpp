#include "HttpListener.h"
#include "HttpSession.h"
#include "SslSession.h"

// Report a failure
static void fail(boost::system::error_code ec, char const* what)
{
    ERR("%s:%s\n", what, ec.message().c_str());
}

HttpListener::HttpListener(boost::asio::io_context& ioc, ssl::context& ctx, bool sslEnable, tcp::endpoint endpoint, shared_ptr<HttpServer>& server)
    : ssl_enable_(sslEnable)
    , ctx_(std::move(ctx))
    , acceptor_(ioc)
{
    mServer = server;
    boost::system::error_code ec;
    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        fail(ec, "open");
        return;
    }
    // Allow address reuse
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        fail(ec, "set_option");
        return;
    }
    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        fail(ec, "bind");
        return;
    }
    // Start listening for connections
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        fail(ec, "listen");
        return;
    }
}

void HttpListener::start()
{
    if (!acceptor_.is_open()) {
        return;
    }
    do_accept();
}

void HttpListener::do_accept()
{
    auto ioc = std::make_shared<boost::asio::io_context>();
    auto socket = std::make_shared<tcp::socket>(*ioc);
    acceptor_.async_accept(*socket, std::bind(&HttpListener::on_accept, shared_from_this(), ioc, socket, std::placeholders::_1));
}

void HttpListener::on_accept(shared_ptr<boost::asio::io_context> ioc, shared_ptr<tcp::socket> socket, boost::system::error_code ec)
{
    if (ec) {
        fail(ec, "accept");
    } else {
        if (ssl_enable_) {
            std::make_shared<SslSession>(ioc, socket, ctx_, mServer)->start();
        } else {
            std::make_shared<HttpSession>(ioc, socket, mServer)->start();
        }
    }
    // Accept another connection
    do_accept();
}
