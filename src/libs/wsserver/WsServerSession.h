#pragma once
#include "WebSocketServer.h"

class WsServerSession : public std::enable_shared_from_this<WsServerSession>, public IWsServerSession {
public:
    WsServerSession(tcp::socket&& socket, WebSocketServerCb cb);
    void start();
    virtual void do_write(string message, bool async = false);
    virtual void do_write(char* data, int len, bool async = false);
    virtual string getRemoteIp();
    virtual int getRemotePort();

private:
    void on_run();
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

private:
    websocket::stream<beast::tcp_stream> mStream;
    beast::flat_buffer mBuffer;
    WebSocketServerCb mCb;
    std::mutex mMutex;
    string mRemoteIp;
    int mRemotePort = 0;
};
