#pragma once

#include "WebSocketClient.h"

class WsClientSession : public std::enable_shared_from_this<WsClientSession>, public IWsClientSession {
public:
    WsClientSession(shared_ptr<net::io_context> ioContext, string host, int port, string path, string query, WebSocketClientCb cb);
    void start();
    void do_write(string message);
    void do_write(char* data, int len);

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void do_close();
    void on_close(beast::error_code ec);

private:
    tcp::resolver mResolver;
    websocket::stream<beast::tcp_stream> mStream;
    beast::flat_buffer mBuffer;
    std::string mHost;
    string mRemoteIp;
    int mRemotePort = 0;
    string mPath;
    string mQuery;
    string mLocalIp;
    int mLocalPort = 0;
    WebSocketClientCb mCb;
};
