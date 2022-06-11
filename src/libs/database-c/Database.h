#pragma once
#include "tools/Singleton.h"
#ifdef WIN32
#include <mysql.h>
#elif __APPLE__
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>

class Database
{
public:
	virtual ~Database();
	int connect(std::string host, int port, std::string name, std::string user, std::string password);
    int connect();
    int execSql(std::string sql);
    int execSql(std::string sql, std::vector<std::map<std::string,std::string>> &result);

private:
   // mysqlpp::Connection *mConnection;
	MYSQL *mConnection;
	
    std::string mIp;
    int mPort;
    std::string mDb;
    std::string mName;
    std::string mPasswd;
	
    std::mutex mMtx;
};
