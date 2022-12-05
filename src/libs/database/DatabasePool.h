#pragma once

#include "database/Database.h"
#include "tools/ConnectionPool.h"
#include "tools/Singleton.h"
#include "tools/cpp_common.h"
#include <memory>

class DatabasePool : public ConnectionPool<Database>, public std::enable_shared_from_this<DatabasePool>, public Singleton<DatabasePool> {
public:
    void setHost(string val);
    void setPort(int val);
    void setDbname(string val);
    void setUser(string val);
    void setPassword(string val);

    int insert(std::string sql);
    int insert(std::string sql, std::vector<std::string> params);
    int execSql(std::string sql);
    int execSql(std::string sql, std::vector<std::string> params);
    int execSql(std::string sql, std::vector<std::map<std::string, any>>& result);
    int execSql(std::string sql, std::vector<std::string> params, std::vector<std::map<std::string, any>>& result);

private:
    string mHost;
    int mPort;
    string mDbname;
    string mUser;
    string mPassword;
};
