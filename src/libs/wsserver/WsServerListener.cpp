#include "WsServerListener.h"
#include "WsServerSession.h"
#include "WsServerSslSession.h"

WsServerListener::WsServerListener(shared_ptr<boost::asio::io_context> ioContext, ssl::context& ctx, bool sslEnable, string host, int port, WebSocketServerCb cb)
    : mAcceptor(*ioContext)
    , mSslCtx(std::move(ctx))
{
    mIoContext = ioContext;
    mSslEnable = sslEnable;
    mHost = host;
    mPort = port;
    mCb = cb;
}

int WsServerListener::start()
{
    beast::error_code ec;
    tcp::endpoint endpoint { net::ip::make_address(mHost), (unsigned short)mPort };
    // Open the acceptor
    mAcceptor.open(endpoint.protocol(), ec);
    if (ec) {
        ERRLN("websocket open error, host:{} port:{} msg:{}\n", mHost, mPort, ec.message());
        return -1;
    }
    // Allow address reuse
    mAcceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        ERRLN("websocket set_option error, host:{} port:{} msg:{}\n", mHost, mPort, ec.message());
        return -1;
    }
    // Bind to the server address
    mAcceptor.bind(endpoint, ec);
    if (ec) {
        ERRLN("websocket bind error, host:{} port:{} msg:{}\n", mHost, mPort, ec.message());
        return -1;
    }
    // Start listening for connections
    mAcceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        ERRLN("websocket listen error, host:{} port:{} msg:{}\n", mHost, mPort, ec.message());
        return -1;
    }
    do_accept();
    return 0;
}

void WsServerListener::do_accept()
{
    // The new connection gets its own strand
    mAcceptor.async_accept(net::make_strand(*mIoContext), beast::bind_front_handler(&WsServerListener::on_accept, shared_from_this()));
}

void WsServerListener::on_accept(beast::error_code ec, tcp::socket socket)
{
    if (ec) {
        ERRLN("websocket accept error, host:{} port:{} msg:{}\n", mHost, mPort, ec.message());
        return;
    }
    if (mSslEnable) {
        // Create the session and run it
        std::make_shared<WsServerSslSession>(std::move(socket), mSslCtx, mCb)->start();
    } else {
        // Create the session and run it
        std::make_shared<WsServerSession>(std::move(socket), mCb)->start();
    }
    // Accept another connection
    do_accept();
}
