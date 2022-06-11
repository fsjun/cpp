#pragma once

#include "WebSocketServer.h"

class WsServerSslSession : public std::enable_shared_from_this<WsServerSslSession>, public IWsServerSession {
public:
    WsServerSslSession(tcp::socket&& socket, ssl::context& ctx, WebSocketServerCb cb);
    void start();
    void do_write(string message);
    virtual void do_write(char* data, int len);
    virtual string getRemoteIp();
    virtual int getRemotePort();

private:
    void on_run();
    void on_handshake(beast::error_code ec);
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

private:
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> mStream;
    beast::flat_buffer mBuffer;
    WebSocketServerCb mCb;
    std::mutex mMutex;
    string mRemoteIp;
    int mRemotePort = 0;
};
