#include "httpclient/HttpSyncClient.h"
#include "boost/beast/core/error.hpp"
#include "boost/format.hpp"
#include "osinfo/NetworkInfo.h"
#include "url/Url.h"
#include <memory>
#include <string>
#include <unistd.h>

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

int HttpSyncClient::httpGet(string path, string& body)
{
    if (mSsl) {
        return mSslSession->httpGet(path, body);
    } else {
        return mSession->httpGet(path, body);
    }
    return 0;
}

int HttpSyncClient::httpPost(string path, string contentType, string content, string& body)
{
    if (mSsl) {
        return mSslSession->httpPost(path, contentType, content, body);
    } else {
        return mSession->httpPost(path, contentType, content, body);
    }
    return 0;
}

int HttpSyncClient::HttpGet(string url_s, string& body)
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
        return client.httpGet(url.path, body);
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
        return client.httpGet(url.path, body);
    } else {
        ERR("invalid http url, url:%s", url_s.c_str());
        return -1;
    }
    return -1;
}

int HttpSyncClient::HttpPost(string url_s, string contentType, string content, string& body)
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
        return client.httpPost(url.path, contentType, content, body);
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
        return client.httpPost(url.path, contentType, content, body);
    } else {
        ERR("invalid http url, url:%s", url_s.c_str());
        return -1;
    }
    return -1;
}
