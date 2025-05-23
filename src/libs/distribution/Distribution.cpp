#include "Distribution.h"
#include "threadpool/DelayedThread.h"
#include <algorithm>
#include <mutex>
#include <vector>
#include "redispp/Redispp.h"

Distribution::~Distribution()
{
}

void Distribution::setExpire(int val)
{
    mExpire = val;
}

void Distribution::setTimeout(long val)
{
    mTimeout = val;
}

void Distribution::setAddr(string val)
{
    mAddr = val;
}

void Distribution::setHttp(string val)
{
    mHttp = val;
}

void Distribution::setReadPrefix(string val)
{
    mReadPrefix = val;
}

void Distribution::setWritePrefix(string val)
{
    mWritePrefix = val;
}

int Distribution::start()
{
    DelayedThread::start();
    postDelayedTask("route", mTimeout, [this]() {
        processFunc();
    });
    return 0;
}

void Distribution::processFunc()
{
    process();
    postDelayedTask("route", mTimeout, [this]() {
        processFunc();
    });
}

void Distribution::process()
{
    refreshWriteKey();
    refreshReadKey();
}

string Distribution::getPrefixAddr()
{
    std::lock_guard<std::mutex> l(mMutex);
    int count = mAddrVec.size();
    if (count == 0) {
        return "";
    }
    if (mIndex >= count) {
        mIndex = 0;
    }
    return mAddrVec.at(mIndex++);;
}

void Distribution::enableWrite()
{
    execute([this](){
        mEnableWrite = true;
        refreshWriteKey();
        string key = std::format("{}{}", mWritePrefix, mAddr);
        INFOLN("distribution write key, key:{}", key);
    });
}

void Distribution::disableWrite()
{
    execute([this](){
        mEnableWrite = false;
        auto redispp = Redispp::GetInstance();
        if (mWritePrefix.empty() || mAddr.empty()) {
            return;
        }
        string key = std::format("{}{}", mWritePrefix, mAddr);
        redispp->del(key);
        INFOLN("distribution del key, key:{}", key);
    });
}

void Distribution::refreshWriteKey()
{
    if (mWritePrefix.empty() || mAddr.empty() || !mEnableWrite) {
        return;
    }
    auto redispp = Redispp::GetInstance();
    string key = std::format("{}{}", mWritePrefix, mAddr);
    if (mJavaCompatible) {
        string field = "\"addr\"";
        string value = std::format("\"{}\"", mAddr);
        redispp->hset(key, field, value);
        if (!mHttp.empty()) {
            string field = "\"http\"";
            string value = std::format("\"{}\"", mHttp);
            redispp->hset(key, field, value);
        }
    }else {
        string field = "addr";
        string value = mAddr;
        redispp->hset(key, field, value);
        if (!mHttp.empty()) {
            string field = "http";
            string value = mHttp;
            redispp->hset(key, field, value);
        }
    }
    string date = Tools::LocalTime();
    redispp->hset(key, "date", date);
    redispp->expire(key, mExpire);
    string keySet = std::format("{}set", mWritePrefix);
    redispp->sadd(keySet, key);
}

void Distribution::refreshReadKey()
{
    if (mReadPrefix.empty()) {
        return;
    }
    auto redispp = Redispp::GetInstance();
    vector<string> addrVec;
    string readKeySet = std::format("{}set", mReadPrefix);
    std::unordered_set<string> result;
    redispp->smembers(readKeySet, result);
    for (auto& val: result) {
        if (val.empty()) {
            continue;
        }
        string key = val;
        std::optional<string> optionalstr;
        if (mJavaCompatible) {
            optionalstr = redispp->hget(key, "\"addr\"");
        } else {
            optionalstr = redispp->hget(key, "addr");
        }
        std::string addr = optionalstr.value_or("");
        if (addr.empty()) {
            continue;
        }
        if (addr.c_str()[0] == '"') {
            addr = addr.substr(1);
        }
        if (addr.c_str()[addr.size() - 1] == '"') {
            addr = addr.substr(0, addr.size() - 1);
        }
        addrVec.emplace_back(addr);
    }
    mergePrefixAddr(addrVec);
}

void Distribution::mergePrefixAddr(vector<string>& addrVec)
{
    std::set<string> removeResult;
    std::set<string> addResult;
    std::lock_guard<std::mutex> l(mMutex);
    std::set<string> newSet(addrVec.begin(), addrVec.end());
    std::set<string> oldSet(mAddrVec.begin(), mAddrVec.end());
    std::set_difference(oldSet.begin(), oldSet.end(), newSet.begin(), newSet.end(), inserter(removeResult, removeResult.begin()));
    std::set_difference(newSet.begin(), newSet.end(), oldSet.begin(), oldSet.end(), inserter(addResult, addResult.begin()));
    if (!removeResult.empty()) {
        std::erase_if(mAddrVec, [&removeResult](auto val){
            return removeResult.contains(val);
        });
    }
    if (!addResult.empty()) {
        mAddrVec.insert(mAddrVec.end(), addResult.begin(), addResult.end());
    }
}
