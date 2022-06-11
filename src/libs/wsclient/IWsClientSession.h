#pragma once

#include <iostream>

using std::string;

class IWsClientSession {
public:
    virtual ~IWsClientSession() = default;
    virtual void do_write(string message) = 0;
    virtual void do_write(char* data, int len) = 0;
};
