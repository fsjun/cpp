#include "redis/RedisPool.h"

void RedisPool::setHost(string val)
{
    mHost = val;
}

void RedisPool::setPort(int val)
{
    mPort = val;
}

void RedisPool::setPassword(string val)
{
    mPassword = val;
}

int RedisPool::execute(const char* format, ...)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Redis> redis) {
        redis->connect(mHost, mPort, mPassword);
    });
    if (!conn) {
        return -1;
    }
    va_list ap;
    va_start(ap, format);
    ret = conn->execute(format, ap);
    va_end(ap);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int RedisPool::execute(string& result, const char* format, ...)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Redis> redis) {
        redis->connect(mHost, mPort, mPassword);
    });
    if (!conn) {
        return -1;
    }
    va_list ap;
    va_start(ap, format);
    ret = conn->execute(result, format, ap);
    va_end(ap);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int RedisPool::execute(map<string, string>& result, const char* format, ...)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Redis> redis) {
        redis->connect(mHost, mPort, mPassword);
    });
    if (!conn) {
        return -1;
    }
    va_list ap;
    va_start(ap, format);
    ret = conn->execute(result, format, ap);
    va_end(ap);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int RedisPool::execute(vector<string>& result, const char* format, ...)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Redis> redis) {
        redis->connect(mHost, mPort, mPassword);
    });
    if (!conn) {
        return -1;
    }
    va_list ap;
    va_start(ap, format);
    ret = conn->execute(result, format, ap);
    va_end(ap);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int RedisPool::execute(Json::Value& result, const char* format, ...)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Redis> redis) {
        redis->connect(mHost, mPort, mPassword);
    });
    if (!conn) {
        return -1;
    }
    va_list ap;
    va_start(ap, format);
    ret = conn->execute(result, format, ap);
    va_end(ap);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}
