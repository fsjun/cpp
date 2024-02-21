
#include "Account.h"

Account::Account(string user, string password, string host, int port)
{
    mEndpoint = Endpoint::GetInstance();
    mUser = user;
    mPassword = password;
    mHost = host;
    mPort = port;
    string contactIp = mEndpoint->getContactIp();
    string transport_param = ";transport=udp";
    mContact = boost::str(boost::format("sip:%s@%s:%d%s") % user % contactIp % mEndpoint->mPort % transport_param);
    mRegUri = boost::str(boost::format("sip:%s:%d") % host % port);
    mUri = boost::str(boost::format("sip:%s@%s:%d") % user % host % port);
    mCredCnt = 1;
    mCred[0].realm = pj_str("*");
    mCred[0].scheme = pj_str("digest");
    mCred[0].username = { (char*)mUser.c_str(), (long)mUser.size() };
    mCred[0].data_type = 0;
    mCred[0].data = { (char*)mPassword.c_str(), (long)mPassword.size() };
    pj_bzero(&mKaTimer, sizeof(mKaTimer));
}

Account::~Account()
{
}

void Account::setCb(function<void(string uri, int status, int expiration, int code, string reason)> cb)
{
    mCb = cb;
}

int Account::initRegc()
{
    pj_status_t status;
    status = pjsip_regc_create(mEndpoint->mEndpt, this, &Account::RegcCb, &mRegc);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to create client registration", status);
        return status;
    }

    pj_str_t reg_uri = { (char*)mRegUri.c_str(), (long)mRegUri.size() };
    pj_str_t uri = { (char*)mUri.c_str(), (long)mUri.size() };
    pj_str_t reg_contact = { (char*)mContact.c_str(), (long)mContact.size() };
    status = pjsip_regc_init(mRegc, &reg_uri, &uri, &uri, 1, &reg_contact, 90);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Client registration initialization error", status);
        pjsip_regc_destroy(mRegc);
        mRegc = NULL;
        return status;
    }
    pjsip_regc_set_reg_tsx_cb(mRegc, Account::RegcTsxCb);
    pjsip_regc_set_credentials(mRegc, mCredCnt, mCred);
    /* Set delay before registration refresh */
    pjsip_regc_set_delay_before_refresh(mRegc, 5);
    string viaIp = mEndpoint->getViaIp();
    pjsip_host_port via_addr = { { (char*)viaIp.c_str(), (long)viaIp.size() }, mEndpoint->mPort };
    pjsip_regc_set_via_sent_by(mRegc, &via_addr, mEndpoint->mTransport);
    return PJ_SUCCESS;
}

int Account::reg()
{
    pjsip_tx_data* tdata = nullptr;
    pj_status_t status;
    status = initRegc();
    if (PJ_SUCCESS != status) {
        return status;
    }
    status = pjsip_regc_register(mRegc, 1, &tdata);
    if (PJ_SUCCESS != status) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "error create regc register tdata", status);
        return status;
    }
    status = pjsip_regc_send(mRegc, tdata);
    if (PJ_SUCCESS != status) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "error send register tdata", status);
        return status;
    }
    return 0;
}

int Account::unreg()
{
    pjsip_tx_data* tdata = nullptr;
    pj_status_t status;
    status = pjsip_regc_unregister(mRegc, &tdata);
    if (PJ_SUCCESS != status) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "error create regc unregister tdata", status);
        return status;
    }
    status = pjsip_regc_send(mRegc, tdata);
    if (PJ_SUCCESS != status) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "error send unregister tdata", status);
        return status;
    }
    return 0;
}

void Account::RegcCb(struct pjsip_regc_cbparam* param)
{
    Account* acc = static_cast<Account*>(param->token);
    if (nullptr != acc) {
        acc->regcCb(param);
    }
}

void Account::regcCb(struct pjsip_regc_cbparam* param)
{
    if (param->status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "SIP registration error", param->status);
        pjsip_regc_destroy(mRegc);
        mRegc = nullptr;
        /* Stop keep-alive timer if any. */
        update_keep_alive(PJ_FALSE, nullptr);
    } else if (param->code < 0 || param->code >= 300) {
        INFO("SIP registration failed, status={} ({:.{}})\n", param->code, param->reason.ptr, param->reason.slen);
        pjsip_regc_destroy(mRegc);
        mRegc = nullptr;
        /* Stop keep-alive timer if any. */
        update_keep_alive(PJ_FALSE, nullptr);
    } else if (PJSIP_IS_STATUS_IN_CLASS(param->code, 200)) {
        if (param->expiration < 1) {
            pjsip_regc_destroy(mRegc);
            mRegc = nullptr;
            /* Stop keep-alive timer if any. */
            update_keep_alive(PJ_FALSE, nullptr);
            INFO("{}: unregistration success\n", mUser);
        } else {
            INFO("{}: registration success, status={} ({:.{}}), will re-register in {} seconds\n", mUri, param->code, param->reason.ptr, param->reason.slen, param->expiration);
            /* Start keep-alive timer if necessary. */
            update_keep_alive(PJ_TRUE, param);
        }
    } else {
        INFO("SIP registration updated status={}\n", param->code);
    }
    auto cb = mCb;
    if (cb) {
        string reason;
        if (param->reason.ptr) {
            reason = { param->reason.ptr, (unsigned long)param->reason.slen };
        }
        cb(mUri, param->status, param->expiration, param->code, reason);
    }
}

void Account::update_keep_alive(pj_bool_t start, struct pjsip_regc_cbparam* param)
{
    /* In all cases, stop keep-alive timer if it's running. */
    if (mKaTimer.id) {
        pjsip_endpt_cancel_timer(mEndpoint->mEndpt, &mKaTimer);
        mKaTimer.id = PJ_FALSE;
    }

    if (start) {
        pj_time_val delay;
        pj_status_t status;

        /* Only do keep-alive if:
         *  - ka_interval is not zero in the account, and
         *  - transport is UDP.
         *
         * Previously we only enabled keep-alive when STUN is enabled, since
         * we thought that keep-alive is only needed in Internet situation.
         * But it has been discovered that Windows Firewall on WinXP also
         * needs to be kept-alive, otherwise incoming packets will be dropped.
         * So because of this, now keep-alive is always enabled for UDP,
         * regardless of whether STUN is enabled or not.
         *
         * Note that this applies only for UDP. For TCP/TLS, the keep-alive
         * is done by the transport layer.
         */
        if (/*pjsua_var.stun_srv.ipv4.sin_family == 0 ||*/
            mKaInterval == 0 || (param->rdata->tp_info.transport->key.type & ~PJSIP_TRANSPORT_IPV6) != PJSIP_TRANSPORT_UDP) {
            /* Keep alive is not necessary */
            return;
        }

        /* https://trac.pjsip.org/repos/ticket/1607:
         * Calculate the destination address from the original request. Some
         * (broken) servers send the response using different source address
         * than the one that receives the request, which is forbidden by RFC
         * 3581.
         */
        {
            pjsip_transaction* tsx;
            pjsip_tx_data* req;

            tsx = pjsip_rdata_get_tsx(param->rdata);
            PJ_ASSERT_ON_FAIL(tsx, return);

            req = tsx->last_tx;

            pj_memcpy(&mKaTarget, &req->tp_info.dst_addr, req->tp_info.dst_addr_len);
            mKaTargetLen = req->tp_info.dst_addr_len;
        }

        /* Setup and start the timer */
        mKaTimer.cb = Account::KeepAliveTimerCb;
        mKaTimer.user_data = (void*)this;

        delay.sec = mKaInterval;
        delay.msec = 0;
        status = pjsip_endpt_schedule_timer(mEndpoint->mEndpt, &mKaTimer, &delay);
        if (status != PJ_SUCCESS) {
            mKaTimer.id = PJ_FALSE;
            mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error starting keep-alive timer", status);
            return;
        }
        char addr[PJ_INET6_ADDRSTRLEN + 10];
        pj_str_t input_str = pj_str(param->rdata->pkt_info.src_name);
        mKaTimer.id = PJ_TRUE;
        pj_addr_str_print(&input_str, param->rdata->pkt_info.src_port, addr, sizeof(addr), 1);
        INFO("Keep-alive timer started for acc {}, destination:{}, interval:{}s\n", mUri, addr, mKaInterval);
    }
}

void Account::RegcTsxCb(struct pjsip_regc_tsx_cb_param* param)
{
    Account* acc = static_cast<Account*>(param->cbparam.token);
    if (nullptr != acc) {
        acc->regcTsxCb(param);
    }
}

void Account::regcTsxCb(struct pjsip_regc_tsx_cb_param* param)
{
}

void Account::KeepAliveTimerCb(pj_timer_heap_t* th, pj_timer_entry* te)
{
    Account* acc = (Account*)te->user_data;
    acc->keepAliveTimerCb(th, te);
}

void Account::keepAliveTimerCb(pj_timer_heap_t* th, pj_timer_entry* te)
{
    pjsip_tpselector tp_sel;
    pj_time_val delay;
    char addrtxt[PJ_INET6_ADDRSTRLEN];
    pj_status_t status;

    /* Select the transport to send the packet */
    pj_bzero(&tp_sel, sizeof(tp_sel));
    tp_sel.type = PJSIP_TPSELECTOR_TRANSPORT;
    tp_sel.u.transport = mEndpoint->mTransport;

    DEBUG("Sending {} bytes keep-alive packet for acc {} to {}\n", mKaData.size(), mUser, pj_sockaddr_print(&mKaTarget, addrtxt, sizeof(addrtxt), 3));

    /* Send raw packet */
    status = pjsip_tpmgr_send_raw(pjsip_endpt_get_tpmgr(mEndpoint->mEndpt), PJSIP_TRANSPORT_UDP, &tp_sel, nullptr, mKaData.c_str(), mKaData.size(), &mKaTarget, mKaTargetLen, nullptr, nullptr);
    if (status != PJ_SUCCESS && status != PJ_EPENDING) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error sending keep-alive packet", status);
    }

    /* Check just in case keep-alive has been disabled. This shouldn't happen
     * though as when ka_interval is changed this timer should have been
     * cancelled.
     */
    if (mKaInterval == 0) {
        return;
    }

    /* Reschedule next timer */
    delay.sec = mKaInterval;
    delay.msec = 0;
    status = pjsip_endpt_schedule_timer(mEndpoint->mEndpt, te, &delay);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error starting keep-alive timer", status);
        return;
    }
    te->id = PJ_TRUE;
}
