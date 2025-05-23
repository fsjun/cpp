#pragma once
#include "boost/beast/http/status.hpp"
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
    static void GenResp(http::request<http::string_body>& req, http::status status, any& resp);

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
    string method = req.method_string();
    string target = req.target();
    auto& body = req.body();
    INFOLN("receive req from {}, method:{} target:{} body:{}", address, method, target, body);
    server->handleRequest(req, resp);
    if (resp.type() == typeid(http::response<http::empty_body>)) {
        auto res = any_cast<http::response<http::empty_body>>(resp);
        int code = res.result_int();
        string reason = res.reason();
        INFOLN("send response to {}, code:{} reason:{}", address, code, reason);
        send(std::move(res));
    } else if (resp.type() == typeid(http::response<http::string_body>)) {
        auto res = any_cast<http::response<http::string_body>>(resp);
        int code = res.result_int();
        string reason = res.reason();
        string& body = res.body();
        INFOLN("send response to {}, code:{} reason:{} body:{}", address, code, reason, body);
        send(std::move(res));
    } else if (resp.type() == typeid(http::response<http::file_body>*)) {
        auto res = any_cast<http::response<http::file_body>*>(resp);
        int code = res->result_int();
        string reason = res->reason();
        INFOLN("send response to {}, code:{} reason:{} file:...", address, code, reason);
        send(std::move(*res));
        delete res;
    } else {
        http::response<http::empty_body> res { http::status::not_acceptable, req.version() };
        res.keep_alive(req.keep_alive());
        int code = res.result_int();
        string reason = res.reason();
        INFOLN("send response to {}, code:{} reason:{}", address, code, reason);
        send(std::move(res));
    }
}
