#pragma once

#include "tools/cpp_common.h"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class HttpSyncClientSession {
public:
    ~HttpSyncClientSession();
    void setHost(string val);
    void setPort(int val);
    int connect();
    void close();
    int httpGet(string path, string& body);
    int httpPost(string path, string contentType, string content, string& body);

private:
    string mHost;
    int mPort;
    net::io_context mIoc;
    unique_ptr<beast::tcp_stream> mStream;
};
