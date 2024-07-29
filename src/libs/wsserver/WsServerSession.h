#pragma once
#include "WebSocketServer.h"
#include <condition_variable>
#include <deque>

class WsServerSession : public std::enable_shared_from_this<WsServerSession>, public IWsServerSession {
public:
    WsServerSession(tcp::socket&& socket, WebSocketServerCb cb);
    void start();
    virtual int do_write(string message, bool async = true);
    virtual int do_write(char* data, int len, bool async = true);
    virtual string getRemoteIp();
    virtual int getRemotePort();

private:
    void on_run();
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(shared_ptr<std::vector<char>> vec, beast::error_code ec, std::size_t bytes_transferred);

private:
    websocket::stream<beast::tcp_stream> mStream;
    beast::flat_buffer mBuffer;
    WebSocketServerCb mCb;
    std::mutex mMutex;
    std::condition_variable mCv;
    bool isSend = false;
    bool mIsSendError = false;
    int mQueueSize = 5;
    std::deque<std::pair<bool, shared_ptr<std::vector<char>>>> mQueue;
    string mRemoteIp;
    int mRemotePort = 0;
};
