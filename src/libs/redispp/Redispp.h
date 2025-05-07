#pragma once

#include "sw/redis++/redis++.h"
#include "tools/Singleton.h"
#include "tools/cpp_common.h"
#include <set>

using namespace sw::redis;
using std::set;
using std::unordered_map;

class Redispp : public Singleton<Redispp> {
public:
    struct options {
        bool cluster;
        string host;
        int port;
        string user;
        string password;
        int db;
        int poolSize;
    };
    void init(options opt);
    void keys(string key, std::set<string>& result);
    bool expire(string key, long long timeout);
    int del(string key);
    std::optional<string> get(string key);

    void hgetall(string key, unordered_map<string, string>& results);
    int hset(string key, string field, string value);
    std::optional<string> hget(string key, string field);
    bool hexists(string key, string field);
    int hdel(string key, string field);

    void hmset(string key, map<string, string> input);
    void hmget(string key, vector<string> fields, vector<std::optional<string>>& results);

private:
    struct options mOptions;
    shared_ptr<Redis> mRedis;
    shared_ptr<RedisCluster> mRedisCluster;
};
