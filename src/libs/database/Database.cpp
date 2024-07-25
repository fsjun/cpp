#include "Database.h"
#include <chrono>
#include <exception>
#include <thread>

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
        ERR("exception on connect mysql: {}\n", e.what());
        mSession.reset();
    } catch (...) {
        ERR("exception on connect mysql\n");
        mSession.reset();
    }
    return ret;
}

int Database::insert(std::string sql)
{
    std::vector<std::any> params;
    return insert(sql, params);
}

int Database::insert(string sql, std::vector<std::any> params)
{
    int id = -1;
    int retry = 0;
    int ret = -1;
    lock_guard<mutex> l(mMtx);
    while (retry++ < mRetryCount) {
        ret = connect();
        if (ret < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        try {
            mysqlx::SqlStatement sqlStatement = mSession->sql(sql);
            ret = bindParams(sqlStatement, params);
            if (ret < 0) {
                ERR("bindParams error\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            mysqlx::SqlResult res = sqlStatement.execute();
            id = res.getAutoIncrementValue();
            ret = 0;
            break;
        } catch (std::exception& e) {
            ERR("sql exception: {}\n", e.what());
            mSession.reset();
            ret = -1;
        } catch (...) {
            ERR("sql exception\n");
            mSession.reset();
            ret = -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return id;
}

int Database::execSql(std::string sql)
{
    std::vector<std::any> params;
    return execSql(sql, params);
}

int Database::execSql(string sql, std::vector<std::any> params)
{
    int count = -1;
    int retry = 0;
    int ret = -1;
    lock_guard<mutex> l(mMtx);
    while (retry++ < mRetryCount) {
        ret = connect();
        if (ret < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        try {
            mysqlx::SqlStatement sqlStatement = mSession->sql(sql);
            ret = bindParams(sqlStatement, params);
            if (ret < 0) {
                ERR("bindParams error\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            mysqlx::SqlResult res = sqlStatement.execute();
            count = res.getAffectedItemsCount();
            ret = 0;
            break;
        } catch (std::exception& e) {
            ERR("sql exception: {}\n", e.what());
            mSession.reset();
            ret = -1;
        } catch (...) {
            ERR("sql exception\n");
            mSession.reset();
            ret = -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return count;
}

int Database::execSql(std::string sql, std::vector<std::map<std::string, any>>& result)
{
    std::vector<std::any> params;
    return execSql(sql, params, result);
}

int Database::execSql(std::string sql, std::vector<std::any> params, std::vector<std::map<std::string, any>>& result)
{
    int retry = 0;
    int ret = -1;
    lock_guard<mutex> l(mMtx);
    while (retry++ < mRetryCount) {
        ret = connect();
        if (ret < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        try {
            mysqlx::SqlStatement sqlStatement = mSession->sql(sql);
            ret = bindParams(sqlStatement, params);
            if (ret < 0) {
                ERR("bindParams error\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            mysqlx::SqlResult res = sqlStatement.execute();
            int rowCount = res.count();
            int colCount = res.getColumnCount();
            auto& columns = res.getColumns();
            for (int i = 0; i < rowCount; ++i) {
                auto row = res.fetchOne();
                std::map<std::string, any> po;
                for (int j = 0; j < colCount; ++j) {
                    auto column = columns[j];
                    string columnLabel = column.getColumnLabel();
                    auto col = row.get(j);
                    auto type = col.getType();
                    switch (type) {
                    case mysqlx::Value::VNULL:
                        break;
                    case mysqlx::Value::UINT64:
                        po.emplace(columnLabel, col.get<uint64_t>());
                        break;
                    case mysqlx::Value::INT64:
                        po.emplace(columnLabel, col.get<int64_t>());
                        break;
                    case mysqlx::Value::FLOAT:
                        po.emplace(columnLabel, col.get<float>());
                        break;
                    case mysqlx::Value::DOUBLE:
                        po.emplace(columnLabel, col.get<double>());
                        break;
                    case mysqlx::Value::BOOL:
                        po.emplace(columnLabel, col.get<bool>());
                        break;
                    case mysqlx::Value::STRING:
                        po.emplace(columnLabel, col.get<string>());
                        break;
                    case mysqlx::Value::DOCUMENT:
                        break;
                    case mysqlx::Value::RAW: {
                        auto byte = col.getRawBytes();
                        uint8_t* data = (uint8_t*)byte.first;
                        int len = byte.second;
                        string str = decodeDateTime(data, len);
                        po.emplace(columnLabel, str);
                        break;
                    }
                    case mysqlx::Value::ARRAY:
                        break;
                    }
                }
                result.emplace_back(po);
            }
            ret = 0;
            break;
        } catch (std::exception& e) {
            ERR("exception on query mysql: {}\n", e.what());
            mSession.reset();
            ret = -1;
        } catch (...) {
            ERR("exception on query mysql\n");
            mSession.reset();
            ret = -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return ret;
}

int Database::execSql(std::string sql, std::vector<std::any> params, Json::Value& result)
{
    int retry = 0;
    int ret = -1;
    lock_guard<mutex> l(mMtx);
    while (retry++ < mRetryCount) {
        ret = connect();
        if (ret < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        try {
            mysqlx::SqlStatement sqlStatement = mSession->sql(sql);
            ret = bindParams(sqlStatement, params);
            if (ret < 0) {
                ERR("bindParams error\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            mysqlx::SqlResult res = sqlStatement.execute();
            int rowCount = res.count();
            int colCount = res.getColumnCount();
            auto& columns = res.getColumns();
            for (int i = 0; i < rowCount; ++i) {
                auto row = res.fetchOne();
                Json::Value po;
                for (int j = 0; j < colCount; ++j) {
                    auto column = columns[j];
                    string columnLabel = column.getColumnLabel();
                    auto col = row.get(j);
                    auto type = col.getType();
                    switch (type) {
                    case mysqlx::Value::VNULL:
                        break;
                    case mysqlx::Value::UINT64:
                        po[columnLabel] = col.get<uint64_t>();
                        break;
                    case mysqlx::Value::INT64:
                        po[columnLabel] = col.get<int64_t>();
                        break;
                    case mysqlx::Value::FLOAT:
                        po[columnLabel] = col.get<float>();
                        break;
                    case mysqlx::Value::DOUBLE:
                        po[columnLabel] = col.get<double>();
                        break;
                    case mysqlx::Value::BOOL:
                        po[columnLabel] = col.get<bool>();
                        break;
                    case mysqlx::Value::STRING:
                        po[columnLabel] = col.get<string>();
                        break;
                    case mysqlx::Value::DOCUMENT:
                        break;
                    case mysqlx::Value::RAW: {
                        auto byte = col.getRawBytes();
                        uint8_t* data = (uint8_t*)byte.first;
                        int len = byte.second;
                        string str = decodeDateTime(data, len);
                        po[columnLabel] = str;
                        break;
                    }
                    case mysqlx::Value::ARRAY:
                        break;
                    }
                }
                result.append(po);
            }
            ret = 0;
            break;
        } catch (std::exception& e) {
            ERR("exception on query mysql: {}\n", e.what());
            mSession.reset();
            ret = -1;
        } catch (...) {
            ERR("exception on query mysql\n");
            mSession.reset();
            ret = -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return ret;
}

int Database::bindParams(mysqlx::SqlStatement& sqlStatement, std::vector<std::any> params)
{
    for (auto& it : params) {
        if (it.type() == typeid(bool)) {
            sqlStatement.bind(any_cast<bool>(it));
        }
        // signed int
        else if (it.type() == typeid(int)) {
            sqlStatement.bind(any_cast<int>(it));
        } else if (it.type() == typeid(long)) {
            sqlStatement.bind(any_cast<long>(it));
        } else if (it.type() == typeid(long long)) {
            sqlStatement.bind(any_cast<long long>(it));
        }
        // unsigned int
        else if (it.type() == typeid(unsigned int)) {
            sqlStatement.bind(any_cast<unsigned int>(it));
        } else if (it.type() == typeid(unsigned long)) {
            sqlStatement.bind(any_cast<unsigned long>(it));
        } else if (it.type() == typeid(unsigned long long)) {
            sqlStatement.bind(any_cast<unsigned long long>(it));
        } else if (it.type() == typeid(float)) {
            sqlStatement.bind(any_cast<float>(it));
        } else if (it.type() == typeid(double)) {
            sqlStatement.bind(any_cast<double>(it));
        }
        // string
        else if (it.type() == typeid(string)) {
            sqlStatement.bind(any_cast<string>(it));
        } else if (it.type() == typeid(char* const)) {
            sqlStatement.bind(any_cast<char* const>(it));
        } else if (it.type() == typeid(char const*)) {
            sqlStatement.bind(any_cast<char const*>(it));
        } else {
            ERR("unsupported std::any type[{}]\n", it.type().name());
            return -1;
        }
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
