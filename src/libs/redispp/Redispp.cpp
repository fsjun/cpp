#include "redispp/Redispp.h"
#include "sw/redis++/errors.h"
#include "sw/redis++/reply.h"
#include <exception>

void Redispp::init(options opt)
{
    try {
        mOptions = opt;
        ConnectionOptions connection_options;
        connection_options.host = opt.host;
        connection_options.port = opt.port;
        if (!opt.user.empty()) {
            connection_options.user = opt.user;
        }
        connection_options.password = opt.password;
        connection_options.db = opt.db;
        connection_options.socket_timeout = std::chrono::milliseconds(200);
        ConnectionPoolOptions pool_options;
        pool_options.size = opt.poolSize;
        pool_options.wait_timeout = std::chrono::milliseconds(100);
        // pool_options.connection_lifetime = std::chrono::minutes(10);
    
        if (mOptions.cluster) {
            mRedisCluster = make_shared<RedisCluster>(connection_options, pool_options);
            auto reply = mRedisCluster->command("CLUSTER", "NODES");
            if (reply && reply::is_string(*reply)) {
                string result = reply->str ? reply->str : "";
                INFOLN("cluster nodes, result:{}", result);
            }
        } else {
            mRedis = make_shared<Redis>(connection_options, pool_options);
        }
    } catch (std::exception& e) {
        ERRLN("error init redis, exception:{}", e.what());
    }
}

void Redispp::keys(string pattern, std::set<string>& result)
{
    try {
        if (mOptions.cluster && mRedisCluster) {
            auto reply = mRedisCluster->command(cmd::keys, pattern);
            reply::to_array(*reply, std::inserter(result, result.begin()));
        } else if (mRedis) {
            mRedis->keys(pattern, std::inserter(result, result.begin()));
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis keys, exception:{}", e.what());
    }
}

bool Redispp::expire(string key, long long timeout)
{
    bool ret = false;
    try {
        if (mOptions.cluster && mRedisCluster) {
            ret = mRedisCluster->expire(key, timeout);
        } else if (mRedis) {
            ret = mRedis->expire(key, timeout);
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis expire, exception:{}", e.what());
    }
    return ret;
}

int Redispp::del(string key)
{
    int ret = -1;
    try {
        if (mOptions.cluster && mRedisCluster) {
            ret = mRedisCluster->del(key);
        } else if (mRedis) {
            ret = mRedis->del(key);
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis del, exception:{}", e.what());
    }
    return ret;
}

std::optional<string> Redispp::get(string key)
{
    std::optional<string> value;
    try {
        if (mOptions.cluster && mRedisCluster) {
            value = mRedisCluster->get(key);
        } else if (mRedis) {
            value = mRedis->get(key);
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis get, exception:{}", e.what());
    }
    return value;
}

void Redispp::hgetall(string key, unordered_map<string, string>& results)
{
    try {
        if (mOptions.cluster && mRedisCluster) {
            mRedisCluster->hgetall(key, std::inserter(results, results.begin()));
        } else if (mRedis) {
            mRedis->hgetall(key, std::inserter(results, results.begin()));
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis hgetall, exception:{}", e.what());
    }
}

int Redispp::hset(string key, string field, string value)
{
    int ret = -1;
    try {
        if (mOptions.cluster && mRedisCluster) {
            ret = mRedisCluster->hset(key, field, value);
        } else if (mRedis) {
            ret = mRedis->hset(key, field, value);
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis hset, exception:{}", e.what());
    }
    return ret;
}

std::optional<string> Redispp::hget(string key, string field)
{
    std::optional<string> value;
    try {
        if (mOptions.cluster && mRedisCluster) {
            value = mRedisCluster->hget(key, field);
        } else if (mRedis) {
            value = mRedis->hget(key, field);
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis hget, exception:{}", e.what());
    }
    return value;
}

bool Redispp::hexists(string key, string field)
{
    bool ret = false;
    try {
        if (mOptions.cluster && mRedisCluster) {
            ret = mRedisCluster->hexists(key, field);
        } else if (mRedis) {
            ret = mRedis->hexists(key, field);
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis hexists, exception:{}", e.what());
    }
    return ret;
}

int Redispp::hdel(string key, string field)
{
    int ret = -1;
    try {
        if (mOptions.cluster && mRedisCluster) {
            ret = mRedisCluster->hdel(key, field);
        } else if (mRedis) {
            ret = mRedis->hdel(key, field);
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis hdel, exception:{}", e.what());
    }
    return ret;
}

void Redispp::hmset(string key, map<string, string> input)
{
    try {
        if (mOptions.cluster && mRedisCluster) {
            mRedisCluster->hmset(key, input.begin(), input.end());
        } else if (mRedis) {
            mRedis->hmset(key, input.begin(), input.end());
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis hmset, exception:{}", e.what());
    }
}

void Redispp::hmget(string key, vector<string> fields, vector<std::optional<string>>& results)
{
    try {
        if (mOptions.cluster && mRedisCluster) {
            mRedisCluster->hmget(key, fields.begin(), fields.end(), std::back_inserter(results));
        } else if (mRedis) {
            mRedis->hmget(key, fields.begin(), fields.end(), std::back_inserter(results));
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis hmget, exception:{}", e.what());
    }
}

int Redispp::sadd(string key, string value)
{
    int ret = -1;
    try {
        if (mOptions.cluster && mRedisCluster) {
            ret = mRedisCluster->sadd(key, value);
        } else if (mRedis) {
            ret = mRedis->sadd(key, value);
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis sadd, exception:{}", e.what());
    }
    return ret;
}

void Redispp::smembers(string key, std::unordered_set<string> result)
{
    try {
        if (mOptions.cluster && mRedisCluster) {
            mRedisCluster->smembers(key, std::inserter(result, result.begin()));
        } else if (mRedis) {
            mRedis->smembers(key, std::inserter(result, result.begin()));
        } else {
            ERRLN("redis connection is nullptr");
        }
    } catch (std::exception& e) {
        ERRLN("error redis smembers, exception:{}", e.what());
    }
}
