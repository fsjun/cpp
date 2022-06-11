#include "AccountService.h"

int AccountService::reg(string user, string password, string host, int port, function<void(string uri, int status, int expiration, int code, string reason)> cb)
{
    int ret = 0;
    string user_uri = boost::str(boost::format("%s@%s:%d") % user % host % port);
    auto acc = make_shared<Account>(user, password, host, port);
    acc->setCb(cb);
    addAccount(user_uri, acc);
    ret = acc->reg();
    if (ret < 0) {
        ERR("reg fail, user:%s password:%s host:%s port:%d\n", user.c_str(), password.c_str(), host.c_str(), port);
        removeAccount(user_uri);
        return -1;
    }
    return 0;
}

int AccountService::unreg(string user_uri)
{
    auto acc = getAccount(user_uri);
    removeAccount(user_uri);
    acc->unreg();
    return 0;
}

void AccountService::addAccount(string user_uri, shared_ptr<Account> acc)
{
    std::lock_guard<mutex> l(mMutex);
    mAccountMap.emplace(user_uri, acc);
}

void AccountService::removeAccount(string user_uri)
{
    std::lock_guard<mutex> l(mMutex);
    mAccountMap.erase(user_uri);
}

shared_ptr<Account> AccountService::getAccount(string user_uri)
{
    std::lock_guard<mutex> l(mMutex);
    auto it = mAccountMap.find(user_uri);
    if (it == mAccountMap.end()) {
        return nullptr;
    }
    return it->second;
}
