#pragma once

#include <iostream>

using std::string;

class IWsServerSession {
public:
    virtual ~IWsServerSession() = default;
    virtual int do_write(string message, bool async = false) = 0;
    virtual int do_write(char* data, int len, bool async = false) = 0;
    virtual string getRemoteIp() = 0;
    virtual int getRemotePort() = 0;
    string sessionId;
};
