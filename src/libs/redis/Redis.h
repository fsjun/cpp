#pragma once
#include <iostream>
#include <json/json.h>
#include <map>
#include <mutex>
#include <sstream>
#include <stdarg.h>
#include <vector>

using std::map;
using std::string;
using std::vector;
struct redisContext;

class Redis {
public:
    virtual ~Redis();
    int connect(std::string ip, int port, string password);
    int execute(const char* format, ...);
    int execute(string& result, const char* format, ...);
    int execute(map<string, string>& result, const char* format, ...);
    int execute(vector<string>& result, const char* format, ...);
    int execute(Json::Value& result, const char* format, ...);

    int execute(const char* format, va_list ap);
    int execute(string& result, const char* format, va_list ap);
    int execute(map<string, string>& result, const char* format, va_list ap);
    int execute(vector<string>& result, const char* format, va_list ap);
    int execute(Json::Value& result, const char* format, va_list ap);

private:
    int connect();
    int disconnect();
    int auth();
    Json::Value getValue(struct redisReply* reply);

private:
    redisContext* mContext;
    std::mutex mMtx;
    std::string mIp;
    int mPort;
    std::string mPassword;
};
