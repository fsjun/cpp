#pragma once

#include "redis/Redis.h"
#include "tools/ConnectionPool.h"
#include "tools/Singleton.h"
#include "tools/cpp_common.h"
#include <memory>

class RedisPool : public ConnectionPool<Redis>, public std::enable_shared_from_this<RedisPool>, public Singleton<RedisPool> {
public:
    void setHost(string val);
    void setPort(int val);
    void setPassword(string val);

    int execute(const char* format, ...);
    int execute(string& result, const char* format, ...);
    int execute(map<string, string>& result, const char* format, ...);
    int execute(vector<string>& result, const char* format, ...);
    int execute(Json::Value& result, const char* format, ...);

private:
    string mHost;
    int mPort = 0;
    string mPassword;
};
