#pragma once

#include "sipserver/Account.h"
#include "tools/Singleton.h"
#include "tools/cpp_common.h"

class AccountService : public Singleton<AccountService> {
public:
    int reg(string user, string password, string host, int port, function<void(string uri, int status, int expiration, int code, string reason)> cb);
    int unreg(string user_uri);

    void addAccount(string user_uri, shared_ptr<Account> acc);
    void removeAccount(string user_uri);
    shared_ptr<Account> getAccount(string user_uri);

private:
    mutex mMutex;
    map<string, shared_ptr<Account>> mAccountMap;
};
