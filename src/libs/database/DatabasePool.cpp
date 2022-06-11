#include "database/DatabasePool.h"
#include <unistd.h>

void DatabasePool::setHost(string val)
{
    mHost = val;
}

void DatabasePool::setPort(int val)
{
    mPort = val;
}

void DatabasePool::setDbname(string val)
{
    mDbname = val;
}

void DatabasePool::setUser(string val)
{
    mUser = val;
}

void DatabasePool::setPassword(string val)
{
    mPassword = val;
}

int DatabasePool::execSql(std::string sql)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Database> db) {
        db->connect(mHost, mPort, mDbname, mUser, mPassword);
    });
    if (!conn) {
        return -1;
    }
    ret = conn->execSql(sql);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int DatabasePool::execSql(std::string sql, std::vector<std::string> params)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Database> db) {
        db->connect(mHost, mPort, mDbname, mUser, mPassword);
    });
    if (!conn) {
        return -1;
    }
    ret = conn->execSql(sql, params);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int DatabasePool::execSql(std::string sql, std::vector<std::map<std::string, any>>& result)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Database> db) {
        db->connect(mHost, mPort, mDbname, mUser, mPassword);
    });
    if (!conn) {
        return -1;
    }
    ret = conn->execSql(sql, result);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int DatabasePool::execSql(std::string sql, std::vector<std::string> params, std::vector<std::map<std::string, any>>& result)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Database> db) {
        db->connect(mHost, mPort, mDbname, mUser, mPassword);
    });
    if (!conn) {
        return -1;
    }
    ret = conn->execSql(sql, params, result);
    revokeConnection(conn);
    if (ret < 0) {
        return -1;
    }
    return 0;
}
