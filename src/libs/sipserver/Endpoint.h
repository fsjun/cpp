#pragma once

#include "log/Log.h"
#include "tools/Singleton.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"
#include <pj/types.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#ifdef _WIN32
#include <Iphlpapi.h>
#include <Winsock2.h>
//#pragma comment(lib, "ws2_32.lib ")
#pragma comment(lib, "Iphlpapi.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/types.h>
#endif

class Account;

class Endpoint : public Singleton<Endpoint>, public std::enable_shared_from_this<Endpoint> {
public:
    int init(int port, string viaIp, string contactIp, int logLevel, bool logOptions);
    virtual ~Endpoint();
    void stop();
    static void PjsipPerror(const char* filename, int line, string title, pj_status_t status);
    string getInterfaceIp();
    bool isInnerIp(struct in_addr* addr);
    int getPort();
    string getViaIp();
    string getContactIp();
    pjsip_transport* getTransport();
    void setLogOptions(bool logOptions);
    void run();
    pj_pool_t* genPool(string name);

    static void CheckPjThread();

    void addAccount(string aor, shared_ptr<Account> account);
    void delAccount(string aor);
    shared_ptr<Account> getAccount(string aor);

public:
    static pj_bool_t mod_pjsip_on_rx_request(pjsip_rx_data* rdata);
    static pj_bool_t mod_pjsip_on_rx_response(pjsip_rx_data* rdata);
    static pj_bool_t mod_dlg_on_rx_request(pjsip_rx_data* rdata);
    static pj_bool_t mod_dlg_on_rx_response(pjsip_rx_data* rdata);
    pj_bool_t call_on_incoming(pjsip_rx_data* rdata);
    pj_bool_t call_on_message(pjsip_rx_data* rdata);
    pj_bool_t call_on_info(pjsip_rx_data* rdata);
    pj_bool_t on_message(pjsip_rx_data* rdata);
    pj_bool_t on_info(pjsip_rx_data* rdata);

    static void on_state_changed(pjsip_inv_session* inv, pjsip_event* e);
    static void on_new_session(pjsip_inv_session* inv, pjsip_event* e);
    static void on_media_update(pjsip_inv_session* inv_ses, pj_status_t status);
    static void on_rx_offer2(pjsip_inv_session* inv, struct pjsip_inv_on_rx_offer_cb_param* param);
    static void on_create_offer(pjsip_inv_session* inv, pjmedia_sdp_session** p_offer);
    static void on_tsx_state_changed(pjsip_inv_session* inv, pjsip_transaction* tsx, pjsip_event* e);
    static pjsip_redirect_op on_redirected(pjsip_inv_session* inv, const pjsip_uri* target, const pjsip_event* e);
    static pj_status_t on_rx_reinvite(pjsip_inv_session* inv, const pjmedia_sdp_session* offer, pjsip_rx_data* rdata);

private:
    int handle_events(unsigned msec_timeout);

private:
    static void log_writer(int level, const char* buffer, int len);

private:
    static bool running;
    pj_caching_pool mCachingPool;
    pj_pool_t* mPool = nullptr;
    pjsip_endpoint* mEndpt = nullptr;
    pjsip_module mAppModule;
    pjsip_module mDlgUsageModule;
    pjsip_transport* mTransport = nullptr;
    int mPort = 6060;
    string mViaIp;
    string mContactIp;
    int mLogLevel = 4;

    mutex mAccLock;
    map<string, shared_ptr<Account>> mAccounts;

    friend class CallService;
    friend class ImService;
    friend class SipCall;
    friend class Account;
};
