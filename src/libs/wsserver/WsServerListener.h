#pragma once
#include "WebSocketServer.h"

class WsServerListener : public std::enable_shared_from_this<WsServerListener> {
public:
    WsServerListener(shared_ptr<boost::asio::io_context> ioContext, ssl::context& ctx, bool sslEnable, string host, int port, WebSocketServerCb cb);
    int start();

private:
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);

private:
    shared_ptr<boost::asio::io_context> mIoContext;
    string mHost;
    int mPort;
    bool mSslEnable = false;
    ssl::context mSslCtx;
    tcp::acceptor mAcceptor;
    WebSocketServerCb mCb;
};
