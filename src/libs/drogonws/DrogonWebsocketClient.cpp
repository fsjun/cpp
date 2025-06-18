#include "DrogonWebsocketClient.h"
#include "DrogonManager.h"
#include <mutex>
#include <string_view>
#include "jsoncc/Jsoncc.h"
#include "url/Url.h"

using namespace drogon;
using namespace std::chrono_literals;

DrogonWebsocketClient::~DrogonWebsocketClient()
{
    INFOLN("DrogonWebsocketClient destruct, client:%d connection:%d id:%s", mClient.get(), mConnection.get(), mId.c_str());
}

void DrogonWebsocketClient::setListener(weak_ptr<DrogonWebsocketListener> val)
{
    mListener = val;
}

void DrogonWebsocketClient::setId(string val)
{
    mId = val;
}

void DrogonWebsocketClient::setUrl(string val)
{
    mUrl = val;
}

void DrogonWebsocketClient::addHeader(string field, string value)
{
    mHeaderMap.emplace(field, value);
}

int DrogonWebsocketClient::start()
{
    string hostString;
    string path;
    auto mgr = DrogonManager::GetInstance();
    mgr->start();
    Url url;
    url.parse(mUrl);
    hostString = url.genHostString();
    path = url.genPathString();
    mClient = WebSocketClient::newWebSocketClient(hostString, nullptr, false, false);
    mRequest = HttpRequest::newHttpRequest();
    mRequest->setPath(path);
    for (auto [key, val] : mHeaderMap) {
        mRequest->addHeader(key, val);
    }
    auto weak = weak_from_this();
    mClient->setMessageHandler([weak](const std::string& message, const WebSocketClientPtr& client, const WebSocketMessageType& type) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        self->onMessage(message, client, type);
    });

    mClient->setConnectionClosedHandler([weak](const WebSocketClientPtr& client) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        self->onClose(client);
    });

    mClient->connectToServer(
        mRequest,
        [weak](ReqResult r, const HttpResponsePtr& response, const WebSocketClientPtr& client) {
            auto self = weak.lock();
            if (!self) {
                return;
            }
            self->onConnect(r, response, client);
        });
    return 0;
}

void DrogonWebsocketClient::stop() {
    std::lock_guard<std::recursive_mutex> l(mMutex);
    if (mIsStop) {
        INFOLN("websocket client is already stopped, id:%s", mId.c_str());
        return;
    }
    mIsStop = true;
    INFOLN("stop websocket client, id:%s", mId.c_str());
    auto self = shared_from_this();
    auto loop = mClient->getLoop();
    if (!loop) {
        ERRLN("client loop is nullptr when stop websocket client, id:%s", mId.c_str());
        return;
    }
    if (!loop->isInLoopThread()) {
        loop->runInLoop([self](){
            self->doStop();
        });
        return;
    }
    doStop();
}

void DrogonWebsocketClient::doStop()
{
    INFOLN("doStop websocket client, id:%s", mId.c_str());
    mClient->stop();
    mClient = nullptr;
    mConnection = nullptr;
}

int DrogonWebsocketClient::send(Json::Value val)
{
    std::lock_guard<std::recursive_mutex> l(mMutex);
    if (mIsStop) {
        ERRLN("client is stopped when send text, id:%s", mId.c_str());
        return -1;
    }
    if (!mConnection) {
        ERRLN("connection is nullptr when send text, id:%s", mId.c_str());
        return -1;
    }
    auto weak = weak_from_this();
    auto loop = mClient->getLoop();
    if (!loop) {
        ERRLN("client loop is nullptr when send text, id:%s", mId.c_str());
        return -1;
    }
    string text = Jsoncc::JsonToString(val);
    INFOLN("websocket client send, text:%s id:%s", text.c_str(), mId.c_str());
    if (!loop->isInLoopThread()) {
        loop->runInLoop([weak, text](){
            auto self = weak.lock();
            if (!self) {
                return;
            }
            self->doSend(text);
        });
        return 0;
    }
    doSend(text);
    return 0;
}

int DrogonWebsocketClient::doSend(string text)
{
    std::lock_guard<std::recursive_mutex> l(mMutex);
    INFOLN("websocket client do send, text:%s id:%s", text.c_str(), mId.c_str());
    if (!mConnection) {
        ERRLN("connection is nullptr when do send text, id:%s", mId.c_str());
        return -1;
    }
    mConnection->send(text);
    return 0;
}

int DrogonWebsocketClient::send(shared_ptr<vector<char>> buff)
{
    std::lock_guard<std::recursive_mutex> l(mMutex);
    if (mIsStop) {
        ERRLN("client is stopped when send binary, id:%s", mId.c_str());
        return -1;
    }
    if (!mConnection) {
        ERRLN("connection is nullptr when send binary, id:%s", mId.c_str());
        return -1;
    }
    auto weak = weak_from_this();
    auto loop = mClient->getLoop();
    if (!loop) {
        ERRLN("client loop is nullptr when send binary, id:%s", mId.c_str());
        return -1;
    }
    if (!loop->isInLoopThread()) {
        loop->runInLoop([weak, buff](){
            auto self = weak.lock();
            if (!self) {
                return;
            }
            self->doSend(buff);
        });
        return 0;
    }
    doSend(buff);
    return 0;
}

int DrogonWebsocketClient::doSend(shared_ptr<vector<char>> buff)
{
    std::lock_guard<std::recursive_mutex> l(mMutex);
    std::string_view sv(buff->data(), buff->size());
    mConnection->send(sv, WebSocketMessageType::Binary);
    return 0;
}

void DrogonWebsocketClient::setConnection(drogon::WebSocketConnectionPtr val)
{
    std::lock_guard<std::recursive_mutex> l(mMutex);
    mConnection = val;
}

void DrogonWebsocketClient::onConnect(drogon::ReqResult r, const drogon::HttpResponsePtr& response, const drogon::WebSocketClientPtr& client)
{
    if (r != ReqResult::Ok) {
        ERRLN("Failed to establish WebSocket connection, id:%s", mId.c_str());
        client->stop();
        auto listener = mListener.lock();
        if (listener) {
            listener->onConnect(r, response);
        }
        return;
    }
    INFOLN("WebSocket connected, id:%s", mId.c_str());
    auto connection = client->getConnection();
    if (!connection) {
        ERRLN("WebSocket connection is nullptr, id:%s", mId.c_str());
        return;
    }
    connection->setPingMessage("", 2s);
    setConnection(connection);
    // client->getConnection()->send("hello!");
    auto listener = mListener.lock();
    if (listener) {
        listener->onConnect(r, response);
    }
}

void DrogonWebsocketClient::onClose(const drogon::WebSocketClientPtr& client)
{
    INFOLN("WebSocket connection closed, id:%s", mId.c_str());
    auto listener = mListener.lock();
    if (listener) {
        listener->onClose();
    }
}

void DrogonWebsocketClient::onMessage(const std::string& message, const drogon::WebSocketClientPtr& client, const drogon::WebSocketMessageType& type)
{
    if (type == WebSocketMessageType::Ping) {
        return;
    }
    if (type == WebSocketMessageType::Pong) {
        return;
    }
    if (type == WebSocketMessageType::Close) {
        INFOLN("receive websocket close, id:%s", mId.c_str());
    }
    if (type == WebSocketMessageType::Text) {
        INFOLN("receive websocket text, text:%s id:%s", message.c_str(), mId.c_str());
    }
    if (type == WebSocketMessageType::Binary) {
        // INFOLN("receive websocket binary, size:%ld id:%s", message.size(), mId.c_str());
    }
    auto listener = mListener.lock();
    if (listener) {
        listener->onMessage(message, type);
    }
}
