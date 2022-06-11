#pragma once

#include <iostream>

using std::string;

class ImListener {
public:
    virtual void on_rx_request(string callId, string method, string body) = 0;
    virtual void on_rx_response(string callId, int code, string reason, string body) = 0;
};
