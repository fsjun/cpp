#include "HttpServer.h"
#include "HttpListener.h"
#include "jsoncc/Jsoncc.h"
#include <regex>
#include <utility>

HttpServer::HttpServer(shared_ptr<boost::asio::io_context> ioContext, string host, int port, bool sslEnable, string caPath, string keyPath, string certPath, shared_ptr<ThreadPool> threadPool)
{
    mIoContext = ioContext;
    mHost = std::move(host);
    mPort = port;
    mSslEnable = sslEnable;
    mCaPath = std::move(caPath);
    mKeyPath = std::move(keyPath);
    mCertPath = std::move(certPath);
    mThreadPool = threadPool;
}

void HttpServer::start()
{
    auto const address = boost::asio::ip::make_address(mHost);
    auto const port = static_cast<unsigned short>(mPort);
    auto self = shared_from_this();
    if (mSslEnable) {
        // The SSL context is required, and holds certificates
        ssl::context ctx { ssl::context::sslv23 };
        ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2);
        ctx.load_verify_file(mCaPath);
        ctx.use_private_key_file(mKeyPath, boost::asio::ssl::context::file_format::pem);
        ctx.use_certificate_file(mCertPath, boost::asio::ssl::context::file_format::pem);
        // Create and launch a listening port
        std::make_shared<HttpListener>(*mIoContext, ctx, true, tcp::endpoint { address, port }, self)->start();
    } else {
        ssl::context ctx { ssl::context::sslv23 };
        // Create and launch a listening port
        std::make_shared<HttpListener>(*mIoContext, ctx, false, tcp::endpoint { address, port }, self)->start();
    }
}

void HttpServer::execute(function<void()> task)
{
    mThreadPool->execute(task);
}

void HttpServer::setFilter(function<int(http::request<http::string_body>& req, any& resp)> filter)
{
    mFilter = filter;
}

void HttpServer::setRoutes(vector<std::pair<string, function<int(http::request<http::string_body>& req, any& resp)>>> routes)
{
    mRoutes = routes;
}

int HttpServer::handleRequest(http::request<http::string_body>& req, any& resp)
{
    if (mFilter) {
        int ret = 0;
        ret = mFilter(req, resp);
        if (ret < 0) {
            return ret;
        }
    }
    string uri = req.target();
    string uri_str = uri;
    int pos = uri_str.find_first_of('?');
    if (pos != string::npos) {
        uri_str = uri_str.substr(0, pos);
    }
    for (auto it : mRoutes) {
        string path = it.first;
        std::regex e(path, std::regex_constants::grep);
        bool match = regex_match(uri_str, e);
        auto cb = it.second;
        if (match) {
            return cb(req, resp);
        }
    }
    return 0;
}

void HttpServer::GenJsonResp(http::request<http::string_body>& req, Json::Value& root, any& resp)
{
    string result = Jsoncc::JsonToString(root);
    http::response<http::string_body> res { http::status::ok, req.version() };
    res.set(http::field::server, "http");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = result;
    res.prepare_payload();
    resp = res;
}
