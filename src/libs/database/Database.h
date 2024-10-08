#pragma once
#include "jsoncc/Jsoncc.h"
#include "log/Log.h"
#include "tools/Singleton.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"
#include <any>

#ifdef WIN32
#include "mysqlx/xdevapi.h"
#elif __APPLE__
#include "mysqlx/xdevapi.h"
#else
#include "mysqlx/xdevapi.h"
#endif
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

class Database {
public:
    virtual ~Database();
    int connect(std::string host, int port, std::string name, std::string user, std::string password);
    int connect();

    int insert(std::string sql);
    int insert(std::string sql, std::vector<std::any> params);

    int execSql(std::string sql);
    int execSql(std::string sql, std::vector<std::any> params);

    int execSql(std::string sql, std::vector<std::map<std::string, any>>& result);
    int execSql(std::string sql, std::vector<std::any> params, std::vector<std::map<std::string, any>>& result);
    int execSql(std::string sql, std::vector<std::any> params, Json::Value& result);

private:
    int bindParams(mysqlx::SqlStatement& sqlStatement, std::vector<std::any> params);
    uint64_t decodeOneData(uint8_t* in, size_t* len);
    std::string decodeDateTime(uint8_t* buffer, size_t size);

private:
    std::unique_ptr<mysqlx::Session> mSession;

    std::string mIp;
    int mPort;
    std::string mDb;
    std::string mName;
    std::string mPasswd;
    std::string mUrl;

    std::mutex mMtx;
    int mRetryCount = 3;
};
