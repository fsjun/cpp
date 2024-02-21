
#define THIS_MODULE "DATABASE"

#include "Database.h"
#include "log/Log.h"

using namespace std;

Database::~Database()
{
    if (mConnection) {
        mysql_close(mConnection);
        mConnection = NULL;
    }
}

int Database::connect(string host, int port, string name, string user, string password)
{
    mIp = host;
    mPort = port;
    mDb = name;
    mName = user;
    mPasswd = password;
    mConnection = NULL;

    return connect();
}

int Database::connect()
{
    if (mConnection == NULL) {
        mConnection = mysql_init(NULL);
        if (!mConnection) {
            ERR("Database::mysql init error\n");
            return -1;
        }
        char argBool = 1;
        mysql_options(mConnection, MYSQL_OPT_RECONNECT, &argBool);

        int argInt = 5;
        mysql_options(mConnection, MYSQL_OPT_CONNECT_TIMEOUT, &argInt);
        mysql_options(mConnection, MYSQL_OPT_READ_TIMEOUT, &argInt);
        mysql_options(mConnection, MYSQL_OPT_WRITE_TIMEOUT, &argInt);

        if (mysql_real_connect(mConnection, mIp.c_str(), mName.c_str(), mPasswd.c_str(), mDb.c_str(), mPort, NULL, 0) == NULL) {
            ERR("Database::mysql real connect error:{}\n", mysql_error(mConnection));

            mysql_close(mConnection);
            mConnection = NULL;

            return -1;
        }
    }
    return 0;
}

int Database::execSql(string sql)
{
    lock_guard<mutex> l(mMtx);
    int ret = connect();
    if (ret < 0) {
        return -1;
    }

    int result = mysql_real_query(mConnection, sql.c_str(), sql.size());
    if (result != 0) {
        ERR("sql[{}], error code:{}:{}\n", sql, result, mysql_error(mConnection));
        return -1;
    }

    return 0;
}

int Database::execSql(string sql, vector<map<string, string>>& result)
{
    lock_guard<mutex> l(mMtx);
    int ret = connect();
    if (ret < 0) {
        return -1;
    }

    int errorCode = mysql_real_query(mConnection, sql.c_str(), sql.size());
    if (errorCode != 0) {
        ERR("sql[{}], error code:{}:{}\n", sql, errorCode, mysql_error(mConnection));
        return -1;
    }

    MYSQL_RES* result_res = NULL;
    MYSQL_FIELD* fields = NULL; // 列名
    MYSQL_ROW row;
    unsigned int fields_num; // 列数
    unsigned long* lengths;

    result_res = mysql_use_result(mConnection);

    if (result_res == NULL) {
        ERR("sql[{}], error:{}\n", sql, mysql_error(mConnection));
        return -1;
    }

    fields_num = mysql_num_fields(result_res);
    fields = mysql_fetch_fields(result_res);

    while (row = mysql_fetch_row(result_res)) {
        lengths = mysql_fetch_lengths(result_res);

        map<string, string> dict;
        for (int i = 0; i < fields_num; i++) {
            /*char *temp = fields[i].name;
            string *key = new string(temp);

            LOG_TO_PRINT(LOG_MODULE_CTI,LOG_LEVEL_DEBUG,"============ %s =========",row[i]);
            if(lengths[i]>0)
            {
                    temp = row[i];
                    string *value = new string(temp);
                    dict[*key] = *value;
                    delete value;
            }
            else
            {
                    dict[*key] = "";
            }

            delete key;*/

            string key = fields[i].name;

            if (lengths[i] > 0) {
                string value = row[i];
                dict[key] = value;
            } else {
                dict[key] = "";
            }
        }

        result.push_back(dict);
    }

    mysql_free_result(result_res);

    return 0;
}
