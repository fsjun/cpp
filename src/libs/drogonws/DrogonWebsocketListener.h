#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/RequestStream.h>
#include <drogon/WebSocketClient.h>

class DrogonWebsocketListener {
public:
    virtual void onConnect(drogon::ReqResult r, const drogon::HttpResponsePtr& response) = 0;
    virtual void onClose() = 0;
    virtual void onMessage(const std::string& message, const drogon::WebSocketMessageType& type) = 0;
};
