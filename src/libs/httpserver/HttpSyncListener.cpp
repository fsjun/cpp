#include "HttpSyncListener.h"
#include "HttpSession.h"
#include "SslSession.h"
#include "osinfo/OsSignal.h"

// Report a failure
static void fail(boost::system::error_code ec, char const* what)
{
    ERRLN("{}:{}", what, ec.message());
}

HttpSyncListener::HttpSyncListener(boost::asio::io_context& ioc, ssl::context& ctx, bool sslEnable, tcp::endpoint endpoint, shared_ptr<HttpServer>& server)
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

void HttpSyncListener::start()
{
    if (!acceptor_.is_open()) {
        return;
    }
    auto self = shared_from_this();
    mServer->execute([self]() {
        self->run();
    });
}

void HttpSyncListener::run()
{
    while (mIsRun) {
        OsSignal::Recover();
        auto ioc = std::make_shared<boost::asio::io_context>();
        auto socket = std::make_shared<tcp::socket>(*ioc);
        boost::system::error_code ec;
        acceptor_.accept(*socket, ec);
        if (ec) {
            fail(ec, "accept");
            continue;
        }
        if (ssl_enable_) {
            std::make_shared<SslSession>(ioc, socket, ctx_, mServer)->start();
        } else {
            std::make_shared<HttpSession>(ioc, socket, mServer)->start();
        }
    }
}
