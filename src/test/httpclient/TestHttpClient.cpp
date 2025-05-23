#include "boostrun/BoostRun.h"
#include "httpclient/HttpClient.h"
#include "log/Log.h"
#include "gtest/gtest.h"
#include <memory>
#include <mutex>

using std::make_shared;
using std::shared_ptr;

class HttpClientTest : public testing::Test {
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

TEST_F(HttpClientTest, http)
{
    string url_s = "http://127.0.0.1:8080/user";
    shared_ptr<BoostRun> boostRun = BoostRun::GetInstance();
    shared_ptr<HttpClient> httpClient = make_shared<HttpClient>(boostRun->getIoContext());
    httpClient->httpGet(url_s, [boostRun](bool success, int code, string reason, string body) {
        boostRun->stop();
    });
    boostRun->run();
}

TEST_F(HttpClientTest, https)
{
    string url_s = "https://127.0.0.1:8080/user";
    shared_ptr<BoostRun> boostRun = BoostRun::GetInstance();
    shared_ptr<HttpClient> httpClient = make_shared<HttpClient>(boostRun->getIoContext());
    httpClient->httpGet(url_s, [boostRun](bool success, int code, string reason, string body) {
        boostRun->stop();
    });
    boostRun->run();
}
