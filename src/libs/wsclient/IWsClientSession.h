#pragma once

#include <iostream>

using std::string;

class IWsClientSession {
public:
    virtual ~IWsClientSession() = default;
    virtual void do_write(string message, bool async = false) = 0;
    virtual void do_write(char* data, int len, bool async = false) = 0;
};
