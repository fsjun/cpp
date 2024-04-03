#include "database/DatabasePool.h"

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

int DatabasePool::insert(std::string sql)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Database> db) {
        db->connect(mHost, mPort, mDbname, mUser, mPassword);
    });
    if (!conn) {
        return -1;
    }
    ret = conn->insert(sql);
    revokeConnection(conn);
    return ret;
}

int DatabasePool::insert(std::string sql, std::vector<std::any> params)
{
    int ret = 0;
    auto conn = getConnection([this](shared_ptr<Database> db) {
        db->connect(mHost, mPort, mDbname, mUser, mPassword);
    });
    if (!conn) {
        return -1;
    }
    ret = conn->insert(sql, params);
    revokeConnection(conn);
    return ret;
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
    return ret;
}

int DatabasePool::execSql(std::string sql, std::vector<std::any> params)
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
    return ret;
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
    return ret;
}

int DatabasePool::execSql(std::string sql, std::vector<std::any> params, std::vector<std::map<std::string, any>>& result)
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
    return ret;
}

int DatabasePool::execSql(std::string sql, std::vector<std::any> params, Json::Value& result)
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
    return ret;
}
