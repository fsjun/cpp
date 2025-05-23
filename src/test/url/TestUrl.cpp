#include "log/Log.h"
#include "url/Url.h"
#include "gtest/gtest.h"
#include <memory>
#include <mutex>

class UrlTest : public testing::Test {
protected:
    // Code here will be called immediately after the constructor (right before each test)
    void SetUp()
    {
        Log::Init(LOG_LEVEL_INFO);
        INFOLN("SetUp\n");
    }

    // Code here will be called immediately after each test (right before the destructor)
    void TearDown()
    {
        INFOLN("TearDown\n");
    }
};

TEST_F(UrlTest, http)
{
    string url_s = "http://www.baidu.com/path?a=b";
    Url url;
    url.parse(url_s);
    INFOLN("protocol:{}, host:{}, port:{}, path:{}, query:{}\n", url.protocol, url.host, url.port, url.path, url.query);
    EXPECT_EQ("http", url.protocol);
    EXPECT_EQ("www.baidu.com", url.host);
    EXPECT_EQ(80, url.port);
    EXPECT_EQ("/path", url.path);
    EXPECT_EQ("?a=b", url.query);
}

TEST_F(UrlTest, http_port)
{
    string url_s = "http://www.baidu.com:8080/path?a=b";
    Url url;
    url.parse(url_s);
    INFOLN("protocol:{}, host:{}, port:{}, path:{}, query:{}\n", url.protocol, url.host, url.port, url.path, url.query);
    EXPECT_EQ("http", url.protocol);
    EXPECT_EQ("www.baidu.com", url.host);
    EXPECT_EQ(8080, url.port);
    EXPECT_EQ("/path", url.path);
    EXPECT_EQ("?a=b", url.query);
}

TEST_F(UrlTest, http_less)
{
    string url_s = "http://www.baidu.com:8080";
    Url url;
    url.parse(url_s);
    INFOLN("protocol:{}, host:{}, port:{}, path:{}, query:{}\n", url.protocol, url.host, url.port, url.path, url.query);
    EXPECT_EQ("http", url.protocol);
    EXPECT_EQ("www.baidu.com", url.host);
    EXPECT_EQ(8080, url.port);
    EXPECT_EQ("", url.path);
    EXPECT_EQ("", url.query);
}

TEST_F(UrlTest, https)
{
    string url_s = "https://www.baidu.com/path?a=b";
    Url url;
    url.parse(url_s);
    INFOLN("protocol:{}, host:{}, port:{}, path:{}, query:{}\n", url.protocol, url.host, url.port, url.path, url.query);
    EXPECT_EQ("https", url.protocol);
    EXPECT_EQ("www.baidu.com", url.host);
    EXPECT_EQ(443, url.port);
    EXPECT_EQ("/path", url.path);
    EXPECT_EQ("?a=b", url.query);
}
