#pragma once

#include <iostream>

using std::string;

class IWsServerSession {
public:
    virtual ~IWsServerSession() = default;
    virtual void do_write(string message) = 0;
    virtual void do_write(char* data, int len) = 0;
    virtual string getRemoteIp() = 0;
    virtual int getRemotePort() = 0;
    string sessionId;
};
