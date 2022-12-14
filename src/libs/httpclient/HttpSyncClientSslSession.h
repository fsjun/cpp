#pragma once

#include "tools/cpp_common.h"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
namespace ssl = net::ssl; // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class HttpSyncClientSslSession {
public:
    ~HttpSyncClientSslSession();
    void setHost(string val);
    void setPort(int val);
    int connect();
    void close();
    int httpGet(string path, vector<map<string, string>> headers, string& body);
    int httpPost(string path, vector<map<string, string>> headers, string contentType, string content, string& body);

private:
    void load_root_certificates(ssl::context& ctx);

private:
    string mHost;
    int mPort;
    net::io_context mIoc;
    unique_ptr<beast::ssl_stream<beast::tcp_stream>> mStream;
};
