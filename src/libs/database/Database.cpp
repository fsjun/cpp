#include "Database.h"
#include <exception>

using namespace std;

Database::~Database()
{
}

int Database::connect(string host, int port, string name, string user, string password)
{
    mIp = host;
    mPort = port;
    mDb = name;
    mName = user;
    mPasswd = password;
    mUrl = boost::str(boost::format("mysqlx://%s:%s@%s:%d/%s") % user % password % host % port % name);
    return connect();
}

int Database::connect()
{
    int ret = -1;
    try {
        if (!mSession) {
            mSession.reset(new mysqlx::Session(mUrl));
        }
        ret = 0;
    } catch (std::exception& e) {
        ERR("exception on connect mysql: %s\n", e.what());
        mSession.reset();
    } catch (...) {
        ERR("exception on connect mysql\n");
        mSession.reset();
    }
    return ret;
}

int Database::execSql(std::string sql)
{
    std::vector<std::string> params;
    return execSql(sql, params);
}

int Database::execSql(string sql, std::vector<std::string> params)
{
    int count = -1;
    lock_guard<mutex> l(mMtx);
    int ret = connect();
    if (ret < 0) {
        return -1;
    }
    try {
        mysqlx::SqlStatement sqlStatement = mSession->sql(sql);
        for (auto& it : params) {
            sqlStatement.bind(it);
        }
        mysqlx::RowResult res = sqlStatement.execute();
        count = res.getAffectedItemsCount();
    } catch (std::exception& e) {
        ERR("sql exception: %s\n", e.what());
        mSession.reset();
    } catch (...) {
        ERR("sql exception\n");
        mSession.reset();
    }
    return count;
}

int Database::execSql(std::string sql, std::vector<std::map<std::string, any>>& result)
{
    std::vector<std::string> params;
    return execSql(sql, params, result);
}

int Database::execSql(std::string sql, std::vector<std::string> params, std::vector<std::map<std::string, any>>& result)
{
    lock_guard<mutex> l(mMtx);
    int ret = connect();
    if (ret < 0) {
        return -1;
    }
    try {
        mysqlx::SqlStatement sqlStatement = mSession->sql(sql);
        for (auto& it : params) {
            sqlStatement.bind(it);
        }
        mysqlx::RowResult res = sqlStatement.execute();
        int rowCount = res.count();
        int colCount = res.getColumnCount();
        auto& columns = res.getColumns();
        for (int i = 0; i < rowCount; ++i) {
            auto row = res.fetchOne();
            std::map<std::string, any> po;
            for (int j = 0; j < colCount; ++j) {
                auto column = columns[j];
                string columnName = column.getColumnName();
                auto col = row.get(j);
                auto type = col.getType();
                switch (type) {
                case mysqlx::Value::VNULL:
                    break;
                case mysqlx::Value::UINT64:
                    po.emplace(columnName, col.get<uint64_t>());
                    break;
                case mysqlx::Value::INT64:
                    po.emplace(columnName, col.get<int64_t>());
                    break;
                case mysqlx::Value::FLOAT:
                    po.emplace(columnName, col.get<float>());
                    break;
                case mysqlx::Value::DOUBLE:
                    po.emplace(columnName, col.get<double>());
                    break;
                case mysqlx::Value::BOOL:
                    po.emplace(columnName, col.get<bool>());
                    break;
                case mysqlx::Value::STRING:
                    po.emplace(columnName, col.get<string>());
                    break;
                case mysqlx::Value::DOCUMENT:
                    break;
                case mysqlx::Value::RAW: {
                    auto byte = col.getRawBytes();
                    uint8_t* data = (uint8_t*)byte.first;
                    int len = byte.second;
                    string str = decodeDateTime(data, len);
                    po.emplace(columnName, str);
                    break;
                }
                case mysqlx::Value::ARRAY:
                    break;
                }
            }
            result.emplace_back(po);
        }
    } catch (std::exception& e) {
        ERR("exception on query mysql: %s\n", e.what());
        mSession.reset();
    } catch (...) {
        ERR("exception on query mysql\n");
        mSession.reset();
    }
    return 0;
}

uint64_t Database::decodeOneData(uint8_t* in, size_t* len)
{
    uint64_t r = 0;
    do {
        uint64_t val = *in;
        r = (r << 7) | (uint64_t)(val & 127);
        in++;
        (*len)--;
    } while ((*len > 0) && (uint64_t(*in) & 128));
    return r;
}

string Database::decodeDateTime(uint8_t* buffer, size_t size)
{
    std::vector<uint8_t> reverseBuffer;
    for (size_t i = 1; i <= size; i++) {
        reverseBuffer.push_back(buffer[size - i]);
    }
    size_t tmpSize = size;
    uint8_t* reverseData = reverseBuffer.data();
    vector<uint64_t> datetime;
    while (tmpSize > 0) {
        uint64_t time = decodeOneData(&reverseData[size - tmpSize], &tmpSize);
        datetime.emplace(datetime.begin(), time);
    }
    if (datetime.size() < 6) {
        return "";
    }
    string str = boost::str(boost::format("%ld-%02ld-%02ld %02ld:%02ld:%02ld") % datetime[0] % datetime[1] % datetime[2] % datetime[3] % datetime[4] % datetime[5]);
    return str;
}
