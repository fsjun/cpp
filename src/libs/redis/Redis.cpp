
#define THIS_MODULE "REDIS"

#include "Redis.h"
#include "log/Log.h"
#include <hiredis/hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define NO_QFORKIMPL
#include <msvs\win32_interop\win32_fixes.h>
#endif

Redis::~Redis()
{
    if (mContext) {
        redisFree(mContext);
        mContext = NULL;
    }
}

int Redis::connect(std::string ip, int port, string password)
{
    mIp = ip;
    mPort = port;
    mPassword = password;
    return connect();
}

int Redis::connect()
{
    mContext = redisConnect(mIp.c_str(), mPort);
    if (mContext == nullptr) {
        ERR("redis Connection error: can't allocate redis context\n");
        return -1;
    }
    if (mContext->err) {
        ERR("redis Connection error: %s\n", mContext->errstr);
        redisFree(mContext);
        mContext = nullptr;
        return -1;
    }
    if (!mPassword.empty()) {
        int ret = auth();
        if (ret < 0) {
            ERR("auth error, password:%s\n", mPassword.c_str());
            redisFree(mContext);
            mContext = nullptr;
            return -1;
        }
    }
    return 0;
}

int Redis::auth()
{
    redisReply* reply = nullptr;
    reply = static_cast<redisReply*>(redisCommand(mContext, "AUTH %s", mPassword.c_str()));
    if (!reply) {
        if (mContext->err == REDIS_ERR_EOF || mContext->err == REDIS_ERR_IO) {
            ERR("auth error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        } else {
            ERR("auth error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        }
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        ERR("error:%.*s\n", reply->len, reply->str);
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    return 0;
}

int Redis::disconnect()
{
    if (mContext) {
        redisFree(mContext);
        mContext = nullptr;
    }
    return 0;
}

int Redis::execute(const char* format, ...)
{
    int ret = 0;
    va_list ap;
    va_start(ap, format);
    ret = execute(format, ap);
    va_end(ap);
    return ret;
}

int Redis::execute(string& result, const char* format, ...)
{
    int ret = 0;
    va_list ap;
    va_start(ap, format);
    ret = execute(result, format, ap);
    va_end(ap);
    return ret;
}

int Redis::execute(map<string, string>& result, const char* format, ...)
{
    int ret = 0;
    va_list ap;
    va_start(ap, format);
    ret = execute(result, format, ap);
    va_end(ap);
    return ret;
}

int Redis::execute(vector<string>& result, const char* format, ...)
{
    int ret = 0;
    va_list ap;
    va_start(ap, format);
    ret = execute(result, format, ap);
    va_end(ap);
    return ret;
}

int Redis::execute(Json::Value& result, const char* format, ...)
{
    int ret = 0;
    va_list ap;
    va_start(ap, format);
    ret = execute(result, format, ap);
    va_end(ap);
    return ret;
}

Json::Value Redis::getValue(struct redisReply* reply)
{
    int i = 0;
    Json::Value root;
    if (!reply) {
        return root;
    }
    if (reply->type == REDIS_REPLY_STRING) {
        root = reply->str;
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        root = std::to_string(reply->integer);
    } else if (reply->type == REDIS_REPLY_ARRAY) {
        int size = reply->elements;
        if (size == 1) {
            struct redisReply* elementField = *(reply->element);
            root = getValue(elementField);
            return root;
        }
        if (size % 2 == 0) {
            for (i = 0; i < size / 2; ++i) {
                struct redisReply* elementField = *(reply->element + i * 2);
                struct redisReply* elementValue = *(reply->element + i * 2 + 1);
                if (!elementField) {
                    continue;
                }
                if (elementField->type == REDIS_REPLY_STRING) {
                    root[elementField->str] = getValue(elementValue);
                } else if (elementField->type == REDIS_REPLY_INTEGER) {
                    root[std::to_string(elementField->integer)] = getValue(elementValue);
                } else if (elementField->type == REDIS_REPLY_ARRAY) {
                    root[i * 2] = getValue(elementField);
                    root[i * 2 + 1] = getValue(elementValue);
                } else {
                    ERR("execute the value type[%d] is error\n", elementField->type);
                }
            }
            return root;
        }
        for (i = 0; i < reply->elements; ++i) {
            struct redisReply* elementField = *(reply->element + i);
            root[i] = getValue(elementField);
        }
        return root;
    } else {
        ERR("execute the value type[%d] is error\n", reply->type);
    }
    return root;
}

int Redis::execute(const char* format, va_list ap)
{
    int ret = 0;
    std::lock_guard<std::mutex> l(mMtx);
    if (mContext == nullptr) {
        ret = connect();
        if (ret < 0) {
            return -1;
        }
    }

    redisReply* reply = nullptr;
    reply = static_cast<redisReply*>(redisvCommand(mContext, format, ap));
    if (!reply) {
        if (mContext->err == REDIS_ERR_EOF || mContext->err == REDIS_ERR_IO) {
            ERR("execute error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        } else {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        }
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        ERR("error:%.*s\n", reply->len, reply->str);
        freeReplyObject(reply);
        return -1;
    }

    freeReplyObject(reply);
    return 0;
}

int Redis::execute(string& result, const char* format, va_list ap)
{
    int ret = 0;
    std::lock_guard<std::mutex> l(mMtx);
    if (mContext == nullptr) {
        ret = connect();
        if (ret < 0) {
            return -1;
        }
    }

    redisReply* reply = nullptr;
    reply = static_cast<redisReply*>(redisvCommand(mContext, format, ap));
    if (!reply) {
        if (mContext->err == REDIS_ERR_EOF || mContext->err == REDIS_ERR_IO) {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        } else {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        }
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        ERR("error:%.*s\n", reply->len, reply->str);
        freeReplyObject(reply);
        return -1;
    }
    if (reply->type == REDIS_REPLY_STRING) {
        result = reply->str;
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        result = std::to_string(reply->integer);
    }
    freeReplyObject(reply);
    return 0;
}

int Redis::execute(map<string, string>& result, const char* format, va_list ap)
{
    int ret = 0;
    std::lock_guard<std::mutex> l(mMtx);
    if (mContext == nullptr) {
        ret = connect();
        if (ret < 0) {
            return -1;
        }
    }

    redisReply* reply = nullptr;
    reply = static_cast<redisReply*>(redisvCommand(mContext, format, ap));
    if (!reply) {
        if (mContext->err == REDIS_ERR_EOF || mContext->err == REDIS_ERR_IO) {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        } else {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        }
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        ERR("error:%.*s\n", reply->len, reply->str);
        freeReplyObject(reply);
        return -1;
    }
    if (reply->elements > 0) {
        int i = 0;
        while (i < reply->elements - 1) {
            struct redisReply* elementField = *(reply->element + i);
            struct redisReply* elementValue = *(reply->element + i + 1);
            if (!elementField || !elementValue) {
                continue;
            }
            if (elementValue->type == REDIS_REPLY_STRING) {
                result[elementField->str] = elementValue->str;
            } else if (elementValue->type == REDIS_REPLY_INTEGER) {
                result[elementField->str] = std::to_string(elementValue->integer);
            } else {
                ERR("execute the value type[%d] is error\n", elementValue->type);
            }
            i += 2;
        }
    }
    freeReplyObject(reply);
    return 0;
}

int Redis::execute(vector<string>& result, const char* format, va_list ap)
{
    int ret = 0;
    std::lock_guard<std::mutex> l(mMtx);
    if (mContext == nullptr) {
        ret = connect();
        if (ret < 0) {
            return -1;
        }
    }

    redisReply* reply = nullptr;
    reply = static_cast<redisReply*>(redisvCommand(mContext, format, ap));
    if (!reply) {
        if (mContext->err == REDIS_ERR_EOF || mContext->err == REDIS_ERR_IO) {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        } else {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        }
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        ERR("error:%.*s\n", reply->len, reply->str);
        freeReplyObject(reply);
        return -1;
    }
    if (reply->elements > 0) {
        int i = 0;
        while (i < reply->elements) {
            struct redisReply* elementField = *(reply->element + i);
            if (!elementField) {
                continue;
            }
            if (elementField->type == REDIS_REPLY_STRING) {
                result.emplace_back(elementField->str);
            } else if (elementField->type == REDIS_REPLY_INTEGER) {
                result.emplace_back(std::to_string(elementField->integer));
            } else {
                ERR("execute the value type[%d] is error\n", elementField->type);
            }
            i++;
        }
    }
    freeReplyObject(reply);
    return 0;
}

int Redis::execute(Json::Value& result, const char* format, va_list ap)
{
    int ret = 0;
    std::lock_guard<std::mutex> l(mMtx);
    if (mContext == nullptr) {
        ret = connect();
        if (ret < 0) {
            return -1;
        }
    }

    redisReply* reply = nullptr;
    reply = static_cast<redisReply*>(redisvCommand(mContext, format, ap));
    va_end(ap);
    if (!reply) {
        if (mContext->err == REDIS_ERR_EOF || mContext->err == REDIS_ERR_IO) {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        } else {
            ERR("del error:%s\n", mContext->errstr);
            disconnect();
            return -1;
        }
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        ERR("error:%.*s\n", reply->len, reply->str);
        freeReplyObject(reply);
        return -1;
    }
    if (reply->elements > 0) {
        result = getValue(reply);
    }
    freeReplyObject(reply);
    return 0;
}
