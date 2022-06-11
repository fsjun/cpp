#pragma once
#include "log/Log.h"
#include "threadpool/ThreadPool.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"
#include "json/json.h"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>
using std::function;
using std::map;
using std::shared_ptr;
using std::string;

class HttpServer : public std::enable_shared_from_this<HttpServer> {
public:
    HttpServer(shared_ptr<boost::asio::io_context> ioContext, string host, int port, bool sslEnable, string caPath, string keyPath, string certPath, shared_ptr<ThreadPool> threadPool);
    void setFilter(function<int(http::request<http::string_body>& req, any& resp)> filter);
    void setRoutes(vector<std::pair<string, function<int(http::request<http::string_body>& req, any& resp)>>> routes);
    void start();
    void execute(function<void()> task);

    int handleRequest(http::request<http::string_body>& req, any& resp);

    static void GenJsonResp(http::request<http::string_body>& req, Json::Value& root, any& resp);

private:
    string mHost;
    int mPort;
    bool mSslEnable;
    string mCaPath;
    string mKeyPath;
    string mCertPath;
    function<int(http::request<http::string_body>& req, any& resp)> mFilter;
    vector<std::pair<string, function<int(http::request<http::string_body>& req, any& resp)>>> mRoutes;
    shared_ptr<boost::asio::io_context> mIoContext;
    shared_ptr<ThreadPool> mThreadPool;
};

template <class Send>
void handle_request(http::request<http::string_body>&& req, Send&& send, string address, shared_ptr<HttpServer>& server)
{
    any resp;
    auto method = req.method_string().to_string();
    auto target = req.target().to_string();
    auto& body = req.body();
    INFO("receive req from %s, method:%s target:%s body:%s\n", address.c_str(), method.c_str(), target.c_str(), body.c_str());
    server->handleRequest(req, resp);
    if (resp.type() == typeid(http::response<http::empty_body>)) {
        auto res = any_cast<http::response<http::empty_body>>(resp);
        int code = res.result_int();
        string reason = res.reason().to_string();
        INFO("send response to %s, code:%d reason:%s\n", address.c_str(), code, reason.c_str());
        send(std::move(res));
    } else if (resp.type() == typeid(http::response<http::string_body>)) {
        auto res = any_cast<http::response<http::string_body>>(resp);
        int code = res.result_int();
        string reason = res.reason().to_string();
        string& body = res.body();
        INFO("send response to %s, code:%d reason:%s body:%s\n", address.c_str(), code, reason.c_str(), body.c_str());
        send(std::move(res));
    } else if (resp.type() == typeid(http::response<http::file_body>*)) {
        auto res = any_cast<http::response<http::file_body>*>(resp);
        int code = res->result_int();
        string reason = res->reason().to_string();
        INFO("send response to %s, code:%d reason:%s file:...\n", address.c_str(), code, reason.c_str());
        send(std::move(*res));
        delete res;
    } else {
        http::response<http::empty_body> res { http::status::ok, req.version() };
        res.keep_alive(req.keep_alive());
        int code = res.result_int();
        string reason = res.reason().to_string();
        INFO("send response to %s, code:%d reason:%s\n", address.c_str(), code, reason.c_str());
        send(std::move(res));
    }
}
