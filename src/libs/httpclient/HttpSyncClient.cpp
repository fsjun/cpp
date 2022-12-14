#include "httpclient/HttpSyncClient.h"
#include "boost/beast/core/error.hpp"
#include "boost/format.hpp"
#include "osinfo/NetworkInfo.h"
#include "url/Url.h"
#include <memory>
#include <string>

void HttpSyncClient::setSsl(bool val)
{
    mSsl = val;
}

void HttpSyncClient::setHost(string val)
{
    mHost = val;
}

void HttpSyncClient::setPort(int val)
{
    mPort = val;
}

int HttpSyncClient::connect()
{
    if (mSsl) {
        mSslSession = std::make_unique<HttpSyncClientSslSession>();
        mSslSession->setHost(mHost);
        mSslSession->setPort(mPort);
        return mSslSession->connect();
    } else {
        mSession = std::make_unique<HttpSyncClientSession>();
        mSession->setHost(mHost);
        mSession->setPort(mPort);
        return mSession->connect();
    }
    return 0;
}

void HttpSyncClient::close()
{
    if (mSsl) {
        mSslSession->close();
    } else {
        mSession->close();
    }
}

int HttpSyncClient::httpGet(string path, vector<map<string, string>> headers, string& body)
{
    if (mSsl) {
        return mSslSession->httpGet(path, headers, body);
    } else {
        return mSession->httpGet(path, headers, body);
    }
    return 0;
}

int HttpSyncClient::httpPost(string path, vector<map<string, string>> headers, string contentType, string content, string& body)
{
    if (mSsl) {
        return mSslSession->httpPost(path, headers, contentType, content, body);
    } else {
        return mSession->httpPost(path, headers, contentType, content, body);
    }
    return 0;
}

int HttpSyncClient::HttpGet(string url_s, vector<map<string, string>> headers, string& body)
{
    Url url;
    int ret = 0;
    url.parse(url_s);
    if ("http" == url.protocol) {
        HttpSyncClient client;
        client.setSsl(false);
        client.setHost(url.host);
        client.setPort(url.port);
        ret = client.connect();
        if (ret < 0) {
            return -1;
        }
        string path = boost::str(boost::format("%s%s") % url.path % url.query);
        return client.httpGet(url.path, headers, body);
    } else if ("https" == url.protocol) {
        HttpSyncClient client;
        client.setSsl(true);
        client.setHost(url.host);
        client.setPort(url.port);
        ret = client.connect();
        if (ret < 0) {
            return -1;
        }
        string path = boost::str(boost::format("%s%s") % url.path % url.query);
        return client.httpGet(url.path, headers, body);
    } else {
        ERR("invalid http url, url:%s", url_s.c_str());
        return -1;
    }
    return -1;
}

int HttpSyncClient::HttpPost(string url_s, vector<map<string, string>> headers, string contentType, string content, string& body)
{
    Url url;
    int ret = 0;
    url.parse(url_s);
    if ("http" == url.protocol) {
        HttpSyncClient client;
        client.setSsl(false);
        client.setHost(url.host);
        client.setPort(url.port);
        ret = client.connect();
        if (ret < 0) {
            return -1;
        }
        string path = boost::str(boost::format("%s%s") % url.path % url.query);
        return client.httpPost(url.path, headers, contentType, content, body);
    } else if ("https" == url.protocol) {
        HttpSyncClient client;
        client.setSsl(true);
        client.setHost(url.host);
        client.setPort(url.port);
        ret = client.connect();
        if (ret < 0) {
            return -1;
        }
        string path = boost::str(boost::format("%s%s") % url.path % url.query);
        return client.httpPost(url.path, headers, contentType, content, body);
    } else {
        ERR("invalid http url, url:%s", url_s.c_str());
        return -1;
    }
    return -1;
}

string HttpSyncClient::GetUserAgent()
{
    //    return BOOST_BEAST_VERSION_STRING;
    return "http client";
}
