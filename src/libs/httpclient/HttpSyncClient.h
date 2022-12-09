#pragma once

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
    int httpGet(string path, string& body);
    int httpPost(string path, string contentType, string content, string& body);

    static int HttpGet(string url, string& body);
    static int HttpPost(string url, string contentType, string content, string& body);

private:
    bool mSsl = false;
    string mHost;
    int mPort;

    unique_ptr<HttpSyncClientSession> mSession;
    unique_ptr<HttpSyncClientSslSession> mSslSession;
};
