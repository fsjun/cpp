#pragma once

#include "drogonws/DrogonWebsocketListener.h"
#include "tools/cpp_common.h"
#include <drogon/WebSocketConnection.h>
#include <memory>
#include <mutex>

class DrogonWebsocketClient : public std::enable_shared_from_this<DrogonWebsocketClient> {
public:
    ~DrogonWebsocketClient();
    void setListener(weak_ptr<DrogonWebsocketListener> val);
    void setId(string val);
    void setUrl(string val);
    void addHeader(string field, string value);
    int start();
    void stop();
    int send(Json::Value val);
    int send(shared_ptr<vector<char>> buff);

private:
    void doStop();
    int doSend(string val);
    int doSend(shared_ptr<vector<char>> buff);

    void setConnection(drogon::WebSocketConnectionPtr val);

private:
    void onConnect(drogon::ReqResult r, const drogon::HttpResponsePtr& response, const drogon::WebSocketClientPtr& client);
    void onClose(const drogon::WebSocketClientPtr& client);
    void onMessage(const std::string& message, const drogon::WebSocketClientPtr& client, const drogon::WebSocketMessageType& type);

private:
    string mId;
    string mUrl;
    map<string,string> mHeaderMap;
    std::recursive_mutex mMutex;
    int mIsStop = false;
    drogon::WebSocketClientPtr mClient;
    drogon::WebSocketConnectionPtr mConnection;
    drogon::HttpRequestPtr mRequest;
    weak_ptr<DrogonWebsocketListener> mListener;
};
