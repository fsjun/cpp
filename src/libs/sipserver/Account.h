#pragma once

#include "Endpoint.h"

class Account : public std::enable_shared_from_this<Account> {
public:
    Account(string user, string password, string host, int port);
    virtual ~Account();
    void setCb(function<void(string uri, int status, int expiration, int code, string reason)> cb);
    int reg();
    int unreg();

private:
    int initRegc();
    static void RegcCb(struct pjsip_regc_cbparam* param);
    void regcCb(struct pjsip_regc_cbparam* param);
    static void RegcTsxCb(struct pjsip_regc_tsx_cb_param* param);
    void regcTsxCb(struct pjsip_regc_tsx_cb_param* param);
    void update_keep_alive(pj_bool_t start, struct pjsip_regc_cbparam* param);
    static void KeepAliveTimerCb(pj_timer_heap_t* th, pj_timer_entry* te);
    void keepAliveTimerCb(pj_timer_heap_t* th, pj_timer_entry* te);

private:
    shared_ptr<Endpoint> mEndpoint;
    pjsip_regc* mRegc = nullptr;
    string mContact;
    string mRegUri;
    string mUri;
    unsigned int mCredCnt = 1;
    pjsip_cred_info mCred[1];

    pj_timer_entry mKaTimer;
    int mKaInterval = 5;
    pj_sockaddr mKaTarget;
    unsigned mKaTargetLen;
    string mKaData = "\r\n\r\n";

    string mUser;
    string mPassword;
    string mHost;
    int mPort = 5060;
    function<void(string uri, int status, int expiration, int code, string reason)> mCb;
};
