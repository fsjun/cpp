
#include "Endpoint.h"
#include "Account.h"
#include "CallService.h"
#include "ImService.h"
#include "PjsipMsgLogger.h"
#include "osinfo/OsSignal.h"
#include "tools/boost_common.h"

bool Endpoint::running = true;

static const char* addr_string(const pj_sockaddr_t* addr)
{
    static char str[128];
    str[0] = '\0';
    pj_inet_ntop(((const pj_sockaddr*)addr)->addr.sa_family,
        pj_sockaddr_get_addr(addr),
        str, sizeof(str));
    return str;
}

/*
 * The INFO method.
 */
const pjsip_method pjsip_info_method = {
    PJSIP_OTHER_METHOD,
    { "INFO", 4 }
};

int Endpoint::init(int port, string viaIp, string contactIp, int logLevel, bool logOptions)
{
    mPort = port;
    mViaIp = viaIp;
    mContactIp = contactIp;
    mLogLevel = logLevel;
    pj_status_t status;

    pj_log_set_log_func(&log_writer);

    /* Set log level */
    pj_log_set_level(mLogLevel);
    PjsipMsgLogger::logOptions = logOptions;

    /* Init PJLIB: */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    pj_log_push_indent();

    /* Init PJLIB-UTIL: */
    status = pjlib_util_init();
    if (status != PJ_SUCCESS) {
        pj_log_pop_indent();
        PjsipPerror(__FILE__, __LINE__, "Failed in initializing pjlib-util:", status);
        pj_shutdown();
        return -1;
    }

    /* Init caching pool. */
    pj_caching_pool_init(&mCachingPool, NULL, 0);
    /* Create memory pool for application. */
    mPool = pj_pool_create(&mCachingPool.factory, "pjsipmodule", 1000, 1000, nullptr);
    if (mPool == NULL) {
        pj_log_pop_indent();
        status = PJ_ENOMEM;
        PjsipPerror(__FILE__, __LINE__, "Unable to create pjsua pool", status);
        pj_shutdown();
        return -1;
    }

    /* Must create SIP endpoint to initialize SIP parser. The parser
     * is needed for example when application needs to call pjsua_verify_url().
     */
    status = pjsip_endpt_create(&mCachingPool.factory, pj_gethostname()->ptr, &mEndpt);
    if (status != PJ_SUCCESS) {
        pj_log_pop_indent();
        PjsipPerror(__FILE__, __LINE__, "Unable to create endpoint", status);
        return -1;
    }
    /* Init SIP UA: */

    /* Initialize transaction layer: */
    status = pjsip_tsx_layer_init_module(mEndpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Initialize UA layer module: */
    status = pjsip_ua_init_module(mEndpt, nullptr);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Initialize Replaces support. */
    status = pjsip_replaces_init_module(mEndpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Initialize 100rel support */
    status = pjsip_100rel_init_module(mEndpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Initialize session timer support */
    status = pjsip_timer_init_module(mEndpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    pjsip_endpt_register_module(mEndpt, &PjsipMsgLogger::pjsip_msg_logger);

    mAppModule = {
        NULL, NULL, /* prev, next.			*/
        { "mod-mcu", 9 }, /* Name.				*/
        -1, /* Id				*/
        PJSIP_MOD_PRIORITY_APPLICATION, /* Priority			*/
        NULL, /* load()				*/
        NULL, /* start()				*/
        NULL, /* stop()				*/
        NULL, /* unload()				*/
        &mod_pjsip_on_rx_request, /* on_rx_request()			*/
        &mod_pjsip_on_rx_response, /* on_rx_response()			*/
        NULL, /* on_tx_request.			*/
        NULL, /* on_tx_response()			*/
        NULL, /* on_tsx_state()			*/
    };

    status = pjsip_endpt_register_module(mEndpt, &mAppModule);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    mDlgUsageModule = {
        NULL, NULL, /* prev, next.			*/
        { "mod-mcu-dlgusage", 9 }, /* Name.				*/
        -1, /* Id				*/
        PJSIP_MOD_PRIORITY_APPLICATION, /* Priority			*/
        NULL, /* load()				*/
        NULL, /* start()				*/
        NULL, /* stop()				*/
        NULL, /* unload()				*/
        &mod_dlg_on_rx_request, /* on_rx_request()			*/
        &mod_dlg_on_rx_response, /* on_rx_response()			*/
        NULL, /* on_tx_request.			*/
        NULL, /* on_tx_response()			*/
        NULL, /* on_tsx_state()			*/
    };
    status = pjsip_endpt_register_module(mEndpt, &mDlgUsageModule);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    pjsip_inv_callback inv_cb;
    const pj_str_t str_norefersub = { "norefersub", 10 };

    /* Initialize invite session callback. */
    pj_bzero(&inv_cb, sizeof(inv_cb));
    inv_cb.on_state_changed = &on_state_changed;
    inv_cb.on_new_session = &on_new_session;
    inv_cb.on_media_update = &on_media_update;
    inv_cb.on_rx_offer2 = &on_rx_offer2;
    inv_cb.on_create_offer = &on_create_offer;
    inv_cb.on_tsx_state_changed = &on_tsx_state_changed;
    inv_cb.on_redirected = &on_redirected;
    inv_cb.on_rx_reinvite = &on_rx_reinvite;

    /* Initialize invite session module: */
    status = pjsip_inv_usage_init(mEndpt, &inv_cb);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Add "norefersub" in Supported header */
    pjsip_endpt_add_capability(mEndpt, NULL, PJSIP_H_SUPPORTED,
        NULL, 1, &str_norefersub);

    /* Add "INFO" in Allow header, for DTMF and video key frame request. */
    pjsip_endpt_add_capability(mEndpt, NULL, PJSIP_H_ALLOW,
        NULL, 1, &pjsip_info_method.name);

    // add transport
    pjsip_transport_type_e type = PJSIP_TRANSPORT_UDP;
    char hostbuf[PJ_INET6_ADDRSTRLEN];
    pj_sock_t sock = PJ_INVALID_SOCKET;
    pj_sockaddr pub_addr;
    pjsip_host_port addr_name;
    int af = pjsip_transport_type_get_af(type);

    /* Initialize the public address from the config, if any */
    pj_sockaddr_init(af, &pub_addr, NULL, (pj_uint16_t)mPort);
    /* Create socket */
    status = pj_sock_socket(af, pj_SOCK_DGRAM(), 0, &sock);
    if (status != PJ_SUCCESS) {
        PjsipPerror(__FILE__, __LINE__, "socket() error", status);
        return -1;
    }
    /* Bind socket */
    status = pj_sock_bind(sock, &pub_addr, pj_sockaddr_get_len(&pub_addr));
    if (status != PJ_SUCCESS) {
        PjsipPerror(__FILE__, __LINE__, "bind() error", status);
        pj_sock_close(sock);
        return -1;
    }
    pj_ansi_strcpy(hostbuf, addr_string(&pub_addr));
    addr_name.host = pj_str(hostbuf);
    addr_name.port = pj_sockaddr_get_port(&pub_addr);
    /* Create UDP transport */
    status = pjsip_udp_transport_attach2(mEndpt, type, sock, &addr_name, 1, &mTransport);
    if (status != PJ_SUCCESS) {
        PjsipPerror(__FILE__, __LINE__, "Error creating SIP UDP transport", status);
        pj_sock_close(sock);
        return -1;
    }
    pjsip_cfg_t* cfg = pjsip_cfg();
    cfg->endpt.disable_tls_switch = 1;
    pj_log_pop_indent();
    return 0;
}

Endpoint::~Endpoint()
{
    CheckPjThread();
    stop();
    pjsip_endpt_destroy(mEndpt);
    pj_pool_release(mPool);
    pj_shutdown();
}

#ifdef _WIN32
string Endpoint::getInterfaceIp()
{
    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
    unsigned long stSize = sizeof(IP_ADAPTER_INFO);
    string ip;
    int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    DWORD netCardNum = 0;
    GetNumberOfInterfaces(&netCardNum);
    netCardNum = 0;
    int IPnumPerNetCard = 0;
    if (ERROR_BUFFER_OVERFLOW == nRel) {
        delete pIpAdapterInfo;
        pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[stSize];
        nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    }
    if (ERROR_SUCCESS == nRel) {
        while (pIpAdapterInfo) {
            switch (pIpAdapterInfo->Type) {
            case MIB_IF_TYPE_OTHER:
                // cout << "OTHER" << endl;
                break;
            case MIB_IF_TYPE_ETHERNET:
                // cout << "ETHERNET" << endl;
                break;
            case MIB_IF_TYPE_TOKENRING:
                // cout << "TOKENRING" << endl;
                break;
            case MIB_IF_TYPE_FDDI:
                // cout << "FDDI" << endl;
                break;
            case MIB_IF_TYPE_PPP:
                // cout << "PPP" << endl;
                break;
            case MIB_IF_TYPE_LOOPBACK:
                // cout << "LOOPBACK" << endl;
                break;
            case MIB_IF_TYPE_SLIP:
                // cout << "SLIP" << endl;
                break;
            default:
                // cout << "" << endl;
                break;
            }
            for (DWORD i = 0; i < pIpAdapterInfo->AddressLength; i++) {
                if (i < pIpAdapterInfo->AddressLength - 1) {
                    // printf("%02X-", pIpAdapterInfo->Address[i]);
                } else {
                    // printf("%02X\n", pIpAdapterInfo->Address[i]);
                }
            }
            IPnumPerNetCard = 0;
            IP_ADDR_STRING* pIpAddrString = &(pIpAdapterInfo->IpAddressList);
            do {
                string ip_str = pIpAddrString->IpAddress.String;
                if (ip_str != "0.0.0.0" && strncmp(ip_str.c_str(), "169.254", 7) != 0 && ip_str != "127.0.0.1") {
                    ip = ip_str;
                }
                pIpAddrString = pIpAddrString->Next;
            } while (pIpAddrString);
            pIpAdapterInfo = pIpAdapterInfo->Next;
            // cout << "--------------------------------------------------------------------" << endl;
        }
    }
    if (pIpAdapterInfo) {
        delete pIpAdapterInfo;
    }
    return ip;
}
#else
string Endpoint::getInterfaceIp()
{
    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* ifa = NULL;
    struct in_addr* addr;
    string ipstr;

    int ret = getifaddrs(&ifAddrStruct);
    if (ret < 0) {
        return "";
    }
    ifa = ifAddrStruct;

    while (ifAddrStruct != NULL) {
        if (ifAddrStruct->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            addr = &((struct sockaddr_in*)ifAddrStruct->ifa_addr)->sin_addr;
            long ip = (long)ntohl(addr->s_addr);
            if (ip != 0x7f000001) {
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, addr, addressBuffer, INET_ADDRSTRLEN);
                cout << "local ip address: " << addressBuffer << endl;
                ipstr = addressBuffer;
                break;
            }
        } else if (ifAddrStruct->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            addr = &((struct sockaddr_in*)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, addr, addressBuffer, INET6_ADDRSTRLEN);
            cout << ("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer) << endl;
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    if (ifa) {
        freeifaddrs(ifa);
    }
    return ipstr;
}
#endif

bool Endpoint::isInnerIp(struct in_addr* addr)
{
#ifdef _WIN32
    long ip = (long)ntohl(addr->S_un.S_addr);
#else
    long ip = (long)ntohl(addr->s_addr);
#endif
    if ((ip >= 0x0A000000 && ip <= 0x0AFFFFFF) || // 10.0.0.0 ~ 10.255.255.255
        (ip >= 0xAC100000 && ip <= 0xAC1FFFFF) || // 172.16.0.0 ~ 172.31.255.255
        (ip >= 0xC0A80000 && ip <= 0xC0A8FFFF) // 192.168.0.0 ~ 192.168.255.255
    ) {
        cout << "Inner ip" << endl;
        return true;
    }
    return false;
}

void Endpoint::stop()
{
    running = false;
}

int Endpoint::getPort()
{
    return mPort;
}

string Endpoint::getViaIp()
{
    return mViaIp;
}

string Endpoint::getContactIp()
{
    return mContactIp;
}

pjsip_transport* Endpoint::getTransport()
{
    return mTransport;
}

void Endpoint::setLogOptions(bool logOptions)
{
    PjsipMsgLogger::logOptions = logOptions;
}

void Endpoint::CheckPjThread()
{
    if (!pj_thread_is_registered()) {
        string thread_name = boost::str(boost::format("endpoint"));
        pj_thread_desc desc;
        pj_bzero(&desc, sizeof(desc));
        pj_thread_t* thread = nullptr;
        pj_thread_register(thread_name.c_str(), desc, &thread);
    }
}

void Endpoint::run()
{
    int timeout = 10;
    int count = 0;
    CheckPjThread();
    OsSignal::Recover();
    while (running) {
        count = handle_events(timeout);
        if (count < 0) {
            pj_thread_sleep(timeout);
        }
    }
}

void Endpoint::addAccount(string aor, shared_ptr<Account> account)
{
    std::lock_guard<mutex> l(mAccLock);
    mAccounts.emplace(aor, account);
}

void Endpoint::delAccount(string aor)
{
    std::lock_guard<mutex> l(mAccLock);
    mAccounts.erase(aor);
}

shared_ptr<Account> Endpoint::getAccount(string aor)
{
    std::lock_guard<mutex> l(mAccLock);
    auto it = mAccounts.find(aor);
    if (it == mAccounts.end()) {
        return nullptr;
    }
    return it->second;
}

int Endpoint::handle_events(unsigned int msec_timeout)
{
    unsigned count = 0;
    pj_time_val tv;
    pj_status_t status;

    tv.sec = 0;
    tv.msec = msec_timeout;
    pj_time_val_normalize(&tv);

    if (!pj_thread_is_registered()) {
        string thread_name = boost::str(boost::format("endpoint"));
        pj_thread_desc desc;
        pj_bzero(&desc, sizeof(desc));
        pj_thread_t* thread = nullptr;
        pj_thread_register(thread_name.c_str(), desc, &thread);
    }

    status = pjsip_endpt_handle_events2(mEndpt, &tv, &count);
    if (status != PJ_SUCCESS) {
        return -status;
    }
    return count;
}

void Endpoint::PjsipPerror(const char* filename, int line, string title, pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE] = { 0 };
    pj_strerror(status, errmsg, sizeof(errmsg));
    std::string file = filename;
    int pos = file.find_last_of("/");
    if (pos != std::string::npos) {
        file = file.substr(pos + 1);
    }
    Log::Print(LOG_LEVEL_ERROR, std::string(__FILE__) + std::string(":") + std::string(CT2STR(__LINE__)), "%s:%d %s %s\n", file.c_str(), line, title.c_str(), errmsg);
}

void Endpoint::log_writer(int level, const char* buffer, int len)
{
    if (!running) {
        return;
    }
    INFO("%.*s\n", len, buffer);
}

/*
 * Handler for receiving incoming requests.
 *
 * This handler serves multiple purposes:
 *  - it receives requests outside dialogs.
 *  - it receives requests inside dialogs, when the requests are
 *    unhandled by other dialog usages. Example of these
 *    requests are: MESSAGE.
 */
pj_bool_t Endpoint::mod_pjsip_on_rx_request(pjsip_rx_data* rdata)
{
    pj_bool_t processed = PJ_FALSE;
    if (!running) {
        return processed;
    }
    auto endpoint = Endpoint::GetInstance();
    pjsip_method_e method = rdata->msg_info.msg->line.req.method.id;
    if (method == PJSIP_INVITE_METHOD) {
        processed = endpoint->call_on_incoming(rdata);
    } else {
        string method = string(rdata->msg_info.msg->line.req.method.name.ptr, rdata->msg_info.msg->line.req.method.name.slen);
        if (method == "MESSAGE") {
            auto endpoint = Endpoint::GetInstance();
            processed = endpoint->on_message(rdata);
        } else if (method == "INFO") {
            auto endpoint = Endpoint::GetInstance();
            processed = endpoint->on_info(rdata);
        } else {
            pjsip_endpt_respond(endpoint->mEndpt, nullptr, rdata, 200, nullptr, nullptr, nullptr, nullptr);
            processed = PJ_TRUE;
        }
    }
    return processed;
}

/*
 * Handler for receiving incoming responses.
 *
 * This handler serves multiple purposes:
 *  - it receives strayed responses (i.e. outside any dialog and
 *    outside any transactions).
 *  - it receives responses coming to a transaction, when pjsua
 *    module is set as transaction user for the transaction.
 *  - it receives responses inside a dialog, when these responses
 *    are unhandled by other dialog usages.
 */
pj_bool_t Endpoint::mod_pjsip_on_rx_response(pjsip_rx_data* rdata)
{
    PJ_UNUSED_ARG(rdata);
    return PJ_FALSE;
}

/*
 * Handler for receiving incoming requests.
 *
 * This handler serves multiple purposes:
 *  - it receives requests outside dialogs.
 *  - it receives requests inside dialogs, when the requests are
 *    unhandled by other dialog usages. Example of these
 *    requests are: MESSAGE.
 */
pj_bool_t Endpoint::mod_dlg_on_rx_request(pjsip_rx_data* rdata)
{
    pj_bool_t processed = PJ_FALSE;
    if (!running) {
        return processed;
    }
    string method = string(rdata->msg_info.msg->line.req.method.name.ptr, rdata->msg_info.msg->line.req.method.name.slen);
    if (method == "MESSAGE") {
        auto endpoint = Endpoint::GetInstance();
        processed = endpoint->call_on_message(rdata);
    } else if (method == "INFO") {
        auto endpoint = Endpoint::GetInstance();
        processed = endpoint->call_on_info(rdata);
    } else if (method == "UPDATE") {

    } else {
        pjsip_transaction* transaction = pjsip_rdata_get_tsx(rdata);
        if (transaction) {
            pjsip_tsx_recv_msg(transaction, rdata);
            pjsip_dialog* dlg = pjsip_rdata_get_dlg(rdata);
            if (dlg) {
                processed = PJ_TRUE;
                pjsip_dlg_respond(dlg, rdata, 200, nullptr, nullptr, nullptr);
            }
        }
    }
    return processed;
}

/*
 * Handler for receiving incoming responses.
 *
 * This handler serves multiple purposes:
 *  - it receives strayed responses (i.e. outside any dialog and
 *    outside any transactions).
 *  - it receives responses coming to a transaction, when pjsua
 *    module is set as transaction user for the transaction.
 *  - it receives responses inside a dialog, when these responses
 *    are unhandled by other dialog usages.
 */
pj_bool_t Endpoint::mod_dlg_on_rx_response(pjsip_rx_data* rdata)
{
    PJ_UNUSED_ARG(rdata);
    return PJ_FALSE;
}

pj_bool_t Endpoint::call_on_incoming(pjsip_rx_data* rdata)
{
    if (!running) {
        return PJ_FALSE;
    }
    pjsip_dialog* dlg = pjsip_rdata_get_dlg(rdata);
    pjsip_transaction* tsx = pjsip_rdata_get_tsx(rdata);
    pjsip_msg* msg = rdata->msg_info.msg;
    /* Don't want to handle anything but INVITE */
    if (msg->line.req.method.id != PJSIP_INVITE_METHOD) {
        return PJ_FALSE;
    }
    /* Don't want to handle anything that's already associated with
     * existing dialog or transaction.
     */
    if (dlg || tsx) {
        return PJ_FALSE;
    }
    if (!running) {
        pjsip_endpt_respond_stateless(mEndpt, rdata, PJSIP_SC_TEMPORARILY_UNAVAILABLE, NULL, NULL, NULL);
        return PJ_TRUE;
    }
    INFO("incoming call: %s\n", rdata->msg_info.info);
    auto callService = CallService::GetInstance();
    callService->on_incoming_call(rdata, &mAppModule);
    return PJ_TRUE;
}

pj_bool_t Endpoint::call_on_message(pjsip_rx_data* rdata)
{
    if (!running) {
        return PJ_FALSE;
    }
    auto callService = CallService::GetInstance();
    callService->on_message(rdata);
    return PJ_TRUE;
}

pj_bool_t Endpoint::call_on_info(pjsip_rx_data* rdata)
{
    if (!running) {
        return PJ_FALSE;
    }
    auto callService = CallService::GetInstance();
    callService->on_info(rdata);
    return PJ_TRUE;
}

pj_pool_t* Endpoint::genPool(string name)
{
    pj_status_t status;
    pj_pool_t* pool = pj_pool_create(&mCachingPool.factory, name.c_str(), 1000, 1000, nullptr);
    if (pool == nullptr) {
        status = PJ_ENOMEM;
        PjsipPerror(__FILE__, __LINE__, "Unable to create pjsua pool", status);
        return nullptr;
    }
    return pool;
}

void Endpoint::on_state_changed(pjsip_inv_session* inv, pjsip_event* e)
{
    if (!running) {
        return;
    }
    auto callService = CallService::GetInstance();
    callService->on_state_changed(inv, e);
}

void Endpoint::on_new_session(pjsip_inv_session* inv, pjsip_event* e)
{
    INFO("on_new_session\n");
}

void Endpoint::on_media_update(pjsip_inv_session* inv_ses, pj_status_t status)
{
    INFO("on_media_update\n");
}

void Endpoint::on_rx_offer2(pjsip_inv_session* inv, struct pjsip_inv_on_rx_offer_cb_param* param)
{
    INFO("on_rx_offer2\n");
    if (!running) {
        return;
    }
    auto callService = CallService::GetInstance();
    callService->on_rx_offer2(inv, param);
}

void Endpoint::on_create_offer(pjsip_inv_session* inv, pjmedia_sdp_session** p_offer)
{
    INFO("on_create_offer\n");
}

void Endpoint::on_tsx_state_changed(pjsip_inv_session* inv, pjsip_transaction* tsx, pjsip_event* e)
{
    // INFO("on_tsx_state_changed\n");
}

pjsip_redirect_op Endpoint::on_redirected(pjsip_inv_session* inv, const pjsip_uri* target, const pjsip_event* e)
{
    INFO("on_redirected\n");
    return PJSIP_REDIRECT_REJECT;
}

pj_status_t Endpoint::on_rx_reinvite(pjsip_inv_session* inv, const pjmedia_sdp_session* offer, pjsip_rx_data* rdata)
{
    INFO("on_rx_reinvite\n");
    return !PJ_SUCCESS;
}

pj_bool_t Endpoint::on_message(pjsip_rx_data* rdata)
{
    if (!running) {
        return PJ_FALSE;
    }
    auto imService = ImService::GetInstance();
    imService->on_message(rdata);
    return PJ_TRUE;
}

pj_bool_t Endpoint::on_info(pjsip_rx_data* rdata)
{
    if (!running) {
        return PJ_FALSE;
    }
    auto imService = ImService::GetInstance();
    imService->on_info(rdata);
    return PJ_TRUE;
}
