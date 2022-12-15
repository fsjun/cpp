#pragma once

#include "boost/beast/http/verb.hpp"
#include "httpclient/HttpSyncClientSession.h"
#include "httpclient/HttpSyncClientSslSession.h"
#include "tools/cpp_common.h"

class HttpSyncClient {
public:
    void setSsl(bool val);
    void setHost(string val);
    void setPort(int val);
    int connect();
    void close();
    int httpGet(string path, vector<map<string, string>> headers, string& body);
    int httpPost(string path, vector<map<string, string>> headers, string contentType, string content, string& body);
    int httpMethod(string path, boost::beast::http::verb method, vector<map<string, string>> headers, string contentType, string content, string& body);

    static int HttpGet(string url, vector<map<string, string>> headers, string& body);
    static int HttpPost(string url, vector<map<string, string>> headers, string contentType, string content, string& body);
    static int HttpMethod(string url, boost::beast::http::verb method, vector<map<string, string>> headers, string contentType, string content, string& body);
    static string GetUserAgent();

private:
    bool mSsl = false;
    string mHost;
    int mPort;

    unique_ptr<HttpSyncClientSession> mSession;
    unique_ptr<HttpSyncClientSslSession> mSslSession;
};
