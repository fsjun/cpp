#pragma once

#include "WebSocketServer.h"
#include <deque>

class WsServerSslSession : public std::enable_shared_from_this<WsServerSslSession>, public IWsServerSession {
public:
    WsServerSslSession(tcp::socket&& socket, ssl::context& ctx, WebSocketServerCb cb);
    void start();
    virtual int do_write(string message, bool async = false);
    virtual int do_write(char* data, int len, bool async = false);
    virtual string getRemoteIp();
    virtual int getRemotePort();

private:
    void on_run();
    void on_handshake(beast::error_code ec);
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(shared_ptr<std::vector<char>> vec, beast::error_code ec, std::size_t bytes_transferred);

private:
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> mStream;
    beast::flat_buffer mBuffer;
    WebSocketServerCb mCb;
    std::mutex mMutex;
    std::condition_variable mCv;
    bool isSend = false;
    int mQueueSize = 5;
    std::deque<std::pair<bool, shared_ptr<std::vector<char>>>> mQueue;
    string mRemoteIp;
    int mRemotePort = 0;
};
