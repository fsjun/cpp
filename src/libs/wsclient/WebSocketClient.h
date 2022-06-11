#pragma once

#include "IWsClientSession.h"
#include "log/Log.h"
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using std::function;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::string;

enum WsClientEventType {
    WS_CLIENT_CONNECT,
    WS_CLIENT_MESSAGE,
    WS_CLIENT_BINARY,
    WS_CLIENT_ERROR,
    WS_CLIENT_CLOSE
};

typedef function<void(shared_ptr<IWsClientSession> session, WsClientEventType type, char* data, int len)> WebSocketClientCb;

class WebSocketClient {
public:
    WebSocketClient(shared_ptr<boost::asio::io_context> ioContext);
    int connect(string url_s, WebSocketClientCb cb);
    static string GetUserAgent();

private:
    void load_root_certificates(ssl::context& ctx);

private:
    shared_ptr<net::io_context> mIoContext;
};
