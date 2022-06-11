#pragma once

#include "IWsServerSession.h"
#include "log/Log.h"
#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

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

class WsServerListener;

enum WsServerEventType {
    WS_SERVER_CONNECT,
    WS_SERVER_MESSAGE,
    WS_SERVER_BINARY,
    WS_SERVER_ERROR,
    WS_SERVER_CLOSE
};

typedef function<void(shared_ptr<IWsServerSession> session, WsServerEventType type, char* data, int len)> WebSocketServerCb;

class WebSocketServer : public std::enable_shared_from_this<WebSocketServer> {
public:
    WebSocketServer(shared_ptr<boost::asio::io_context> ioContext, string host, int port, bool sslEnable, string caPath, string keyPath, string certPath, WebSocketServerCb cb);
    int start();

private:
    string mHost;
    int mPort;
    bool mSslEnable;
    string mCaPath;
    string mKeyPath;
    string mCertPath;
    shared_ptr<boost::asio::io_context> mIoContext;
    shared_ptr<WsServerListener> mWebSocketListener;
    WebSocketServerCb mCb;
};
