
#include "CallService.h"
#include "Endpoint.h"
#include "pj/string.h"
#include "pj/types.h"
#include "pjmedia/sdp.h"
#include "pjsip/sip_msg.h"
#include "pjsip/sip_uri.h"
#include "sipserver/SipCall.h"
#include "tools/Tools.h"
#include "tools/boost_common.h"
#include "json/json.h"
#include <cstring>

const pjsip_method CallService::pjsip_message_method = {
    (pjsip_method_e)PJSIP_OTHER_METHOD,
    { "MESSAGE", 7 }
};

CallService::CallService()
{
    mEndpoint = Endpoint::GetInstance();
}

int CallService::create_contact_by_config(string user, string contactIp, string& contact)
{
    string host;
    if (contactIp.empty()) {
        host = mEndpoint->getContactIp();
    } else {
        host = contactIp;
    }
    int port = mEndpoint->getPort();
    if (user.empty()) {
        contact = std::format("<sip:{}:{}>", host, port);
    } else {
        contact = std::format("<sip:{}@{}:{}>", user, host, port);
    }
    return 0;
}

int CallService::create_contact_by_rdate(pj_pool_t* pool, pj_str_t* contact, pjsip_rx_data* rdata)
{
    /*
     *  Section 12.1.1, paragraph about using SIPS URI in Contact.
     *  If the request that initiated the dialog contained a SIPS URI
     *  in the Request-URI or in the top Record-Route header field value,
     *  if there was any, or the Contact header field if there was no
     *  Record-Route header field, the Contact header field in the response
     *  MUST be a SIPS URI.
     */
    pjsip_sip_uri* sip_uri;
    pj_status_t status;
    pjsip_transport_type_e tp_type = PJSIP_TRANSPORT_UNSPECIFIED;
    pj_str_t local_addr;
    pjsip_tpselector tp_sel;
    pjsip_tpmgr* tpmgr;
    pjsip_tpmgr_fla2_param tfla2_prm;
    unsigned flag;
    int secure;
    int local_port;
    const char *beginquote, *endquote;
    char transport_param[32];
    pj_str_t contact_uri_params = pj_str("");
    pj_str_t contact_params = pj_str("");

    /* If Record-Route is present, then URI is the top Record-Route. */
    if (rdata->msg_info.record_route) {
        sip_uri = (pjsip_sip_uri*)
            pjsip_uri_get_uri(rdata->msg_info.record_route->name_addr.uri);
    } else {
        pjsip_hdr* pos = NULL;
        pjsip_contact_hdr* h_contact;
        pjsip_uri* uri = NULL;

        /* Otherwise URI is Contact URI.
         * Iterate the Contact URI until we find sip: or sips: scheme.
         */
        do {
            h_contact = (pjsip_contact_hdr*)
                pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_CONTACT,
                    pos);
            if (h_contact) {
                if (h_contact->uri)
                    uri = (pjsip_uri*)pjsip_uri_get_uri(h_contact->uri);
                else
                    uri = NULL;
                if (!uri || (!PJSIP_URI_SCHEME_IS_SIP(uri) && !PJSIP_URI_SCHEME_IS_SIPS(uri))) {
                    pos = (pjsip_hdr*)h_contact->next;
                    if (pos == &rdata->msg_info.msg->hdr)
                        h_contact = NULL;
                } else {
                    break;
                }
            }
        } while (h_contact);

        /* Or if Contact URI is not present, take the remote URI from
         * the From URI.
         */
        if (uri == NULL)
            uri = (pjsip_uri*)pjsip_uri_get_uri(rdata->msg_info.from->uri);

        /* Can only do sip/sips scheme at present. */
        if (!PJSIP_URI_SCHEME_IS_SIP(uri) && !PJSIP_URI_SCHEME_IS_SIPS(uri))
            return PJSIP_EINVALIDREQURI;

        sip_uri = (pjsip_sip_uri*)pjsip_uri_get_uri(uri);
    }

    /* Get transport type of the URI */
    if (PJSIP_URI_SCHEME_IS_SIPS(sip_uri))
        tp_type = PJSIP_TRANSPORT_TLS;
    else if (sip_uri->transport_param.slen == 0) {
        tp_type = PJSIP_TRANSPORT_UDP;
    } else
        tp_type = pjsip_transport_get_type_from_name(&sip_uri->transport_param);

    if (tp_type == PJSIP_TRANSPORT_UNSPECIFIED)
        return -1;

    /* If destination URI specifies IPv6 or account is configured to use IPv6
     * or the transport being used to receive data is an IPv6 transport,
     * then set transport type to use IPv6 as well.
     */
    if (pj_strchr(&sip_uri->host, ':') || (rdata->tp_info.transport->key.type & PJSIP_TRANSPORT_IPV6)) {
        tp_type = (pjsip_transport_type_e)(((int)tp_type) | PJSIP_TRANSPORT_IPV6);
    }

    flag = pjsip_transport_get_flag_from_type(tp_type);
    secure = (flag & PJSIP_TRANSPORT_SECURE) != 0;

    pj_bzero(&tp_sel, sizeof(tp_sel));
    tp_sel.type = PJSIP_TPSELECTOR_TRANSPORT;
    tp_sel.u.transport = mEndpoint->getTransport();

    /* Get local address suitable to send request from */
    pjsip_tpmgr_fla2_param_default(&tfla2_prm);
    tfla2_prm.tp_type = tp_type;
    tfla2_prm.tp_sel = &tp_sel;
    tfla2_prm.dst_host = sip_uri->host;
    tfla2_prm.local_if = PJ_TRUE;

    tpmgr = pjsip_endpt_get_tpmgr(mEndpoint->mEndpt);
    status = pjsip_tpmgr_find_local_addr2(tpmgr, pool, &tfla2_prm);
    if (status != PJ_SUCCESS)
        return -1;

    local_addr = tfla2_prm.ret_addr;
    local_port = tfla2_prm.ret_port;

    /* Enclose IPv6 address in square brackets */
    if (tp_type & PJSIP_TRANSPORT_IPV6) {
        beginquote = "[";
        endquote = "]";
    } else {
        beginquote = endquote = "";
    }

    /* Don't add transport parameter if it's UDP */
    if (tp_type != PJSIP_TRANSPORT_UDP && tp_type != PJSIP_TRANSPORT_UDP6) {
        pj_ansi_snprintf(transport_param, sizeof(transport_param),
            ";transport=%s",
            pjsip_transport_get_type_name(tp_type));
    } else {
        transport_param[0] = '\0';
    }

    /* Create the contact header */
    contact->ptr = (char*)pj_pool_alloc(pool, PJSIP_MAX_URL_SIZE);
    contact->slen = pj_ansi_snprintf(contact->ptr, PJSIP_MAX_URL_SIZE,
        "<%s:%.*s%s%s%.*s%s:%d%s%.*s>%.*s",
        ((secure) ? "sips" : "sip"),
        (int)sip_uri->user.slen,
        sip_uri->user.ptr,
        (sip_uri->user.slen ? "@" : ""),
        beginquote,
        (int)local_addr.slen,
        local_addr.ptr,
        endquote,
        local_port,
        transport_param,
        (int)contact_uri_params.slen,
        contact_uri_params.ptr,
        (int)contact_params.slen,
        contact_params.ptr);
    if (contact->slen < 1 || contact->slen >= (int)PJSIP_MAX_URL_SIZE)
        return -1;

    return 0;
}

string CallService::getFromUser(pjsip_rx_data* rdata)
{
    pjsip_name_addr* name_addr = (pjsip_name_addr*)rdata->msg_info.from->uri;
    pjsip_sip_uri* sip_uri = (pjsip_sip_uri*)name_addr->uri;
    return string(sip_uri->user.ptr, sip_uri->user.slen);
}

string CallService::getToUser(pjsip_rx_data* rdata)
{
    pjsip_name_addr* name_addr = (pjsip_name_addr*)rdata->msg_info.to->uri;
    pjsip_sip_uri* sip_uri = (pjsip_sip_uri*)name_addr->uri;
    return string(sip_uri->user.ptr, sip_uri->user.slen);
}

pj_status_t CallService::on_incoming_call(pjsip_rx_data* rdata, pjsip_module* mod)
{
    string contact;
    pj_str_t contact_pj;
    pjsip_dialog* dlg = nullptr;
    pjsip_transaction* tsx = nullptr;
    pjsip_msg* msg = rdata->msg_info.msg;
    pjsip_tx_data* response = nullptr;
    unsigned options = 0;
    pjsip_inv_session* inv = nullptr;
    int call_id = -1;
    int sip_err_code = PJSIP_SC_INTERNAL_SERVER_ERROR;
    pjmedia_sdp_session* offer = nullptr;
    string remoteSdpStr;
    int ret;
    string callId;
    pj_bool_t should_dec_dlg = PJ_FALSE;
    pj_status_t status = PJ_SUCCESS;
    char cname_buf[16] = { 0 };
    shared_ptr<Json::Value> event;
    auto sipCall = make_shared<SipCall>();
    auto endpoint = Endpoint::GetInstance();
    string fromUser = getFromUser(rdata);
    string toUser = getToUser(rdata);
    map<string, string> hdrs;
    pjsip_host_port via_addr = { { (char*)mEndpoint->mViaIp.c_str(), (long)mEndpoint->mViaIp.size() }, mEndpoint->mPort };
    sipCall->callDirection = CALL_DIRECTION_INBOUND;

    /* Generate per-session RTCP CNAME, according to RFC 7022. */
    pj_create_random_string(cname_buf, sizeof(cname_buf));
    /* Mark call start time. */
    sipCall->startTime = Tools::Now();
    pjsip_msg_body* body = rdata->msg_info.msg->body;
    /* Parse SDP from incoming request */
    if (body) {
        pjsip_rdata_sdp_info* sdp_info;
        sdp_info = pjsip_rdata_get_sdp_info(rdata);
        offer = sdp_info->sdp;
        status = sdp_info->sdp_err;
        if (status == PJ_SUCCESS && sdp_info->sdp == nullptr && !PJSIP_INV_ACCEPT_UNKNOWN_BODY) {
            if (sdp_info->body.ptr == nullptr) {
                status = PJSIP_ERRNO_FROM_SIP_STATUS(PJSIP_SC_UNSUPPORTED_MEDIA_TYPE);
            } else {
                status = PJSIP_ERRNO_FROM_SIP_STATUS(PJSIP_SC_NOT_ACCEPTABLE);
            }
        }

        if (status != PJ_SUCCESS) {
            pjsip_hdr hdr_list;
            /* Check if body really contains SDP. */
            if (sdp_info->body.ptr == NULL) {
                /* Couldn't find "application/sdp" */
                pjsip_accept_hdr* acc;
                mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unknown Content-Type in incoming INVITE", status);

                /* Add Accept header to response */
                acc = pjsip_accept_hdr_create(rdata->tp_info.pool);
                PJ_ASSERT_RETURN(acc, PJ_ENOMEM);
                acc->values[acc->count++] = pj_str("application/sdp");
                pj_list_init(&hdr_list);
                pj_list_push_back(&hdr_list, acc);

                pjsip_endpt_respond(mEndpoint->mEndpt, nullptr, rdata, PJSIP_SC_UNSUPPORTED_MEDIA_TYPE, nullptr, &hdr_list, nullptr, nullptr);
            } else {
                const pj_str_t reason = pj_str("Bad SDP");
                pjsip_warning_hdr* w;
                mEndpoint->PjsipPerror(__FILE__, __LINE__, "Bad SDP in incoming INVITE", status);
                w = pjsip_warning_hdr_create_from_status(rdata->tp_info.pool, pjsip_endpt_name(mEndpoint->mEndpt), status);
                pj_list_init(&hdr_list);
                pj_list_push_back(&hdr_list, w);
                pjsip_endpt_respond(mEndpoint->mEndpt, nullptr, rdata, 400, &reason, &hdr_list, nullptr, nullptr);
            }
            goto on_return;
        }

        /* Do quick checks on SDP before passing it to transports. More elabore
         * checks will be done in pjsip_inv_verify_request2() below.
         */
        if ((offer) && (offer->media_count == 0)) {
            const pj_str_t reason = pj_str("Missing media in SDP");
            pjsip_endpt_respond(mEndpoint->mEndpt, nullptr, rdata, 400, &reason, nullptr, nullptr, nullptr);
            goto on_return;
        }
        remoteSdpStr = string((char*)body->data, body->len);
        sipCall->remoteSdp = remoteSdpStr;
    } else {
        offer = nullptr;
    }

    fix_contact_hdr(rdata);
    /* Verify that we can handle the request. */
    options |= PJSIP_INV_SUPPORT_TIMER;
    status = pjsip_inv_verify_request2(rdata, &options, offer, nullptr, nullptr, mEndpoint->mEndpt, &response);
    if (status != PJ_SUCCESS) {
        /*
         * No we can't handle the incoming INVITE request.
         */
        if (response) {
            pjsip_response_addr res_addr;
            pjsip_get_response_addr(response->pool, rdata, &res_addr);
            pjsip_endpt_send_response(mEndpoint->mEndpt, &res_addr, response, nullptr, nullptr);
        } else {
            /* Respond with 500 (Internal Server Error) */
            pjsip_endpt_respond(mEndpoint->mEndpt, nullptr, rdata, 500, nullptr, nullptr, nullptr, nullptr);
        }
        goto on_return;
    }

    status = create_contact_by_config(toUser, "", contact);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to generate Contact header", status);
        pjsip_endpt_respond_stateless(mEndpoint->mEndpt, rdata, 500, nullptr, nullptr, nullptr);
        goto on_return;
    }
    contact_pj.ptr = (char*)contact.c_str();
    contact_pj.slen = contact.size();
    /* Create dialog: */
    status = pjsip_dlg_create_uas_and_inc_lock(pjsip_ua_instance(), rdata, &contact_pj, &dlg);
    if (status != PJ_SUCCESS) {
        pjsip_endpt_respond_stateless(mEndpoint->mEndpt, rdata, 500, nullptr, nullptr, nullptr);
        goto on_return;
    }

    /* If 100rel is optional and UAC supports it, use it. */
    if ((options & PJSIP_INV_REQUIRE_100REL) == 0) {
        const pj_str_t token = { "100rel", 6 };
        pjsip_dialog_cap_status cap_status;

        cap_status = pjsip_dlg_remote_has_cap(dlg, PJSIP_H_SUPPORTED, nullptr, &token);
        if (cap_status == PJSIP_DIALOG_CAP_SUPPORTED) {
            options |= PJSIP_INV_REQUIRE_100REL;
        }
    }

    /* Create invite session: */
    status = pjsip_inv_create_uas(dlg, rdata, nullptr, options, &inv);
    if (status != PJ_SUCCESS) {
        pjsip_hdr hdr_list;
        pjsip_warning_hdr* w;
        w = pjsip_warning_hdr_create_from_status(dlg->pool, pjsip_endpt_name(mEndpoint->mEndpt), status);
        pj_list_init(&hdr_list);
        pj_list_push_back(&hdr_list, w);
        pjsip_dlg_respond(dlg, rdata, 500, nullptr, &hdr_list, nullptr);
        /* Can't terminate dialog because transaction is in progress.
        pjsip_dlg_terminate(dlg);
         */
        goto on_return;
    }

    /* Create and attach pjsua_var data to the dialog */
    sipCall->inv = inv;

    /* Store variables required for the callback after the async
     * media transport creation is completed.
     */
    sipCall->dlg = dlg;

    pjsip_dlg_inc_session(dlg, mod);
    should_dec_dlg = PJ_TRUE;

    /* Init Session Timers */
    status = pjsip_timer_init_session(inv, &sipCall->timerSetting);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Session Timer init failed", status);
        pjsip_dlg_respond(dlg, rdata, PJSIP_SC_INTERNAL_SERVER_ERROR, nullptr, nullptr, nullptr);
        pjsip_inv_terminate(inv, PJSIP_SC_INTERNAL_SERVER_ERROR, PJ_FALSE);
        sipCall->inv = nullptr;
        sipCall->dlg = nullptr;
        goto on_return;
    }

    /* Must answer with some response to initial INVITE. We'll do this before
     * attaching the call to the invite session/dialog, so that the application
     * will not get notification about this event (on another scenario, it is
     * also possible that inv_send_msg() fails and causes the invite session to
     * be disconnected. If we have the call attached at this time, this will
     * cause the disconnection callback to be called before on_incoming_call()
     * callback is called, which is not right).
     */
    status = pjsip_inv_initial_answer(inv, rdata, 100, nullptr, nullptr, &response);
    if (status != PJ_SUCCESS) {
        if (response == nullptr) {
            mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to send answer to incoming INVITE", status);
            pjsip_dlg_respond(dlg, rdata, 500, nullptr, nullptr, nullptr);
            pjsip_inv_terminate(inv, 500, PJ_FALSE);
        } else {
            pjsip_inv_send_msg(inv, response);
            pjsip_inv_terminate(inv, response->msg->line.status.code, PJ_FALSE);
        }
        sipCall->inv = nullptr;
        sipCall->dlg = nullptr;
        goto on_return;
    } else {
        status = pjsip_inv_send_msg(inv, response);
        if (status != PJ_SUCCESS) {
            mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to send 100 response", status);
            sipCall->inv = nullptr;
            sipCall->dlg = nullptr;
            goto on_return;
        }
    }
    pjsip_dlg_set_via_sent_by(dlg, &via_addr, (pjsip_transport*)mEndpoint->mTransport);
    callId = string(dlg->call_id->id.ptr, dlg->call_id->id.slen);

    sipCall->callId = (char*)pj_pool_alloc(sipCall->pool, callId.size() + 1);
    std::memcpy(sipCall->callId, callId.c_str(), callId.size());
    sipCall->callId[callId.size()] = '\0';

    for (pjsip_hdr* hdr = rdata->msg_info.msg->hdr.next; hdr != &rdata->msg_info.msg->hdr; hdr = hdr->next) {
        if (PJSIP_H_OTHER != hdr->type) {
            continue;
        }
        pjsip_generic_string_hdr* generic_hdr = (pjsip_generic_string_hdr*)hdr;
        string hname = string(generic_hdr->name.ptr, generic_hdr->name.slen);
        string hvalue = string(generic_hdr->hvalue.ptr, generic_hdr->hvalue.slen);
        hdrs.emplace(hname, hvalue);
    }
    sipCall->fromUser = fromUser;
    sipCall->toUser = toUser;
    sipCall->hdrs = hdrs;

    ret = addCall(callId, sipCall);
    if (ret < 0) {
        ERR("call in max exceed, current:{} max:{} callId:{}\n", mCurrCallCnt, mMaxCall, callId);
        pjsip_dlg_respond(dlg, rdata, 503, nullptr, nullptr, nullptr);
        pjsip_inv_terminate(inv, 503, PJ_FALSE);
        sipCall->inv = nullptr;
        sipCall->dlg = nullptr;
        goto on_return;
    }
    should_dec_dlg = PJ_FALSE;
    pjsip_dlg_add_usage(dlg, &mEndpoint->mDlgUsageModule, nullptr);
    /* Only do this after sending 100/Trying (really! see the long comment
     * above)
     */
    if (dlg->mod_data[endpoint->mAppModule.id] == nullptr) {
        /* In PJSUA2, on_incoming_call() may be called from
         * on_media_transport_created() hence this might already set
         * to allow notification about fail events via on_call_state() and
         * on_call_tsx_state().
         */
        dlg->mod_data[endpoint->mAppModule.id] = sipCall->callId;
        inv->mod_data[endpoint->mAppModule.id] = sipCall->callId;
    }
    INFO("incoming call, callId:{}\n", callId);
    {
        sipCall->sourceIp = rdata->pkt_info.src_name;
        sipCall->sourcePort = rdata->pkt_info.src_port;
        auto callListener = getCallListener();
        if (callListener) {
            callListener->onIncomingCall(sipCall);
        }
    }
    /* This INVITE request has been handled. */
on_return:
    if (dlg) {
        if (should_dec_dlg) {
            pjsip_dlg_dec_session(dlg, mod);
        }
        pjsip_dlg_dec_lock(dlg);
    }
    return status;
}

void CallService::fix_contact_hdr(pjsip_rx_data* rdata)
{
    pjsip_msg* msg = rdata->msg_info.msg;
    pjsip_contact_hdr* c_hdr = (pjsip_contact_hdr*)pjsip_msg_find_hdr(msg, PJSIP_H_CONTACT, NULL);
    if (c_hdr && c_hdr->uri) {
        return;
    }
    pj_pool_t* pool = rdata->tp_info.pool;
    string host = rdata->pkt_info.src_name;
    int port = rdata->pkt_info.src_port;
    c_hdr = pjsip_contact_hdr_create(pool);
    pjsip_sip_uri* uri = pjsip_sip_uri_create(pool, PJ_FALSE);
    c_hdr->uri = (pjsip_uri*)uri;
    pj_strdup2_with_null(pool, &uri->host, host.c_str());
    uri->port = port;
    pjsip_msg_add_hdr(msg, (pjsip_hdr*)c_hdr);
}

void CallService::on_rx_offer2(pjsip_inv_session* inv, struct pjsip_inv_on_rx_offer_cb_param* param)
{
    pj_status_t status;
    pjsip_tx_data* response = nullptr;
    pjsip_rx_data* rdata = (pjsip_rx_data*)param->rdata;
    pjsip_dialog* dlg = pjsip_rdata_get_dlg(rdata);

    if (!dlg) {
        ERR("Unable to get dialog on rx offer\n");
        return;
    }
    string callId = string(dlg->call_id->id.ptr, dlg->call_id->id.slen);
    auto call = getCall(callId);
    if (!call) {
        ERR("Unable to get call on rx offer, callId:{}\n", callId);
        return;
    }
    if (!param->rdata || !param->rdata->msg_info.msg || !param->rdata->msg_info.msg->body) {
        ERR("param has no sdp body on rx offer, callId:{}\n", callId);
        return;
    }
    pjsip_msg_body* body = param->rdata->msg_info.msg->body;
    string remote_sdp_str = string((char*)body->data, body->len);
    string local_sdp_str;
    auto listener = getCallListener();
    if (listener) {
        listener->onRxOffer(call, remote_sdp_str, local_sdp_str);
    }
    if (local_sdp_str.empty()) {
        local_sdp_str = call->localSdp;
    }
    if (local_sdp_str.empty()) {
        ERR("local sdp is empty on rx offer, callId:{}\n", callId);
        return;
    }
    pjmedia_sdp_parse(call->pool, (char*)local_sdp_str.c_str(), local_sdp_str.size(), &call->sdp_session);
    if (!call->sdp_session) {
        ERR("local sdp parse media session error on rx offer, callId:{}\n", callId);
        return;
    }
    status = pjsip_inv_set_local_sdp(call->inv, call->sdp_session);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to set answer", status);
    }
}

void CallService::on_state_changed(pjsip_inv_session* inv, pjsip_event* e)
{
    int ret = 0;
    auto endpoint = Endpoint::GetInstance();
    string stateName = pjsip_inv_state_name(inv->state);
    string callId;
    if (inv->dlg->call_id && inv->dlg->call_id->id.ptr) {
        callId = string(inv->dlg->call_id->id.ptr, inv->dlg->call_id->id.slen);
    }
    if (callId.empty()) {
        char* modCallId = (char*)inv->dlg->mod_data[endpoint->mAppModule.id];
        if (nullptr == modCallId || strlen(modCallId) == 0) {
            ERR("callId of mod_data is nullptr, state:{}\n", stateName);
            return;
        }
        callId = modCallId;
    }
    auto call = getCall(callId);
    if (!call) {
        if (PJSIP_INV_STATE_INCOMING != inv->state) {
            ERR("call not found callId:{} state:{}\n", callId, stateName);
        }
        return;
    }
    INFO("call state changed callId:{} state:{}\n", call->callId, stateName);
    auto callListener = getCallListener();
    switch (inv->state) {
    case PJSIP_INV_STATE_EARLY:
        if (call->ringTime == 0) {
            call->ringTime = Tools::Now();
        }
        if (CALL_DIRECTION_OUTBOUND == call->callDirection) {
            pjsip_msg_body* body = e->body.rx_msg.rdata->msg_info.msg->body;
            if (body) {
                string remoteSdpStr = string((char*)body->data, body->len);
                call->remoteSdp = remoteSdpStr;
            }
        }
        if (callListener) {
            callListener->onEarly(call);
        }
        break;
    case PJSIP_INV_STATE_CONNECTING: {
        stopTimer(call);
        if (call->answerTime == 0) {
            call->answerTime = Tools::Now();
        }
        if (CALL_DIRECTION_OUTBOUND == call->callDirection) {
            pjsip_msg_body* body = e->body.rx_msg.rdata->msg_info.msg->body;
            string remoteSdpStr = string((char*)body->data, body->len);
            call->remoteSdp = remoteSdpStr;
        }
        if (callListener) {
            callListener->onConnecting(call);
        }
        break;
    }
    case PJSIP_INV_STATE_CONFIRMED:
        if (callListener) {
            callListener->onConfirmed(call);
        }
        break;
    case PJSIP_INV_STATE_DISCONNECTED: {
        stopTimer(call);
        if (call->hangupTime == 0) {
            call->hangupTime = Tools::Now();
        }
        call->dlg->mod_data[endpoint->mAppModule.id] = nullptr;
        call->inv->mod_data[endpoint->mAppModule.id] = nullptr;
        delCall(callId);
        int code;
        string reason;
        if (e->type == PJSIP_EVENT_TSX_STATE) {
            code = e->body.tsx_state.tsx->status_code;
            pj_str_t status_text = e->body.tsx_state.tsx->status_text;
            reason = string(status_text.ptr, status_text.slen);
        } else {
            code = PJSIP_SC_REQUEST_TERMINATED;
            const pj_str_t* status_text = pjsip_get_status_text(code);
            reason = string(status_text->ptr, status_text->slen);
        }
        call->code = code;
        call->reason = reason;
        INFO("call hangup, callId:{} code:{} reason:{}\n", call->callId, code, reason);
        if (callListener) {
            callListener->onDisconnected(call);
        }
        break;
    }
    default:
        break;
    }
}

void CallService::on_message(pjsip_rx_data* rdata)
{
    if (!rdata->msg_info.msg->body->data || !rdata->msg_info.msg->body->len) {
        pjsip_endpt_respond(mEndpoint->mEndpt, nullptr, rdata, 200, nullptr, nullptr, nullptr, nullptr);
        return;
    }
    string callId = string(rdata->msg_info.cid->id.ptr, rdata->msg_info.cid->id.slen);
    string body = string((char*)rdata->msg_info.msg->body->data, rdata->msg_info.msg->body->len);
    pjsip_transaction* transaction = pjsip_rdata_get_tsx(rdata);
    INFO("receive message, callId:{} body:{}\n", callId, body);
    pjsip_tsx_recv_msg(transaction, rdata);
    auto call = getCall(callId);
    if (!call) {
        ERR("call is not found, callId:{}\n", callId);
        pjsip_dlg_respond(call->dlg, rdata, 404, nullptr, nullptr, nullptr);
        return;
    }
    pjsip_dlg_respond(call->dlg, rdata, 200, nullptr, nullptr, nullptr);
    auto callListener = getCallListener();
    if (callListener) {
        callListener->onMessage(call, body);
    }
}

void CallService::on_info(pjsip_rx_data* rdata)
{
    if (!rdata->msg_info.msg->body->data || !rdata->msg_info.msg->body->len) {
        pjsip_endpt_respond(mEndpoint->mEndpt, nullptr, rdata, 200, nullptr, nullptr, nullptr, nullptr);
        return;
    }
    string callId = string(rdata->msg_info.cid->id.ptr, rdata->msg_info.cid->id.slen);
    string body = string((char*)rdata->msg_info.msg->body->data, rdata->msg_info.msg->body->len);
    pjsip_transaction* transaction = pjsip_rdata_get_tsx(rdata);
    INFO("receive message, callId:{} body:{}\n", callId, body);
    pjsip_tsx_recv_msg(transaction, rdata);
    auto call = getCall(callId);
    if (!call) {
        ERR("call is not found, callId:{}\n", callId);
        pjsip_dlg_respond(call->dlg, rdata, 404, nullptr, nullptr, nullptr);
        return;
    }
    pjsip_dlg_respond(call->dlg, rdata, 200, nullptr, nullptr, nullptr);
    auto callListener = getCallListener();
    if (callListener) {
        callListener->onInfo(call, body);
    }
}

int CallService::makeCall(shared_ptr<SipCall> call, string callId, string from, string to, string host, int timeout, string viaIp, string contactIp, string local_sdp, map<string, string>& hdrs, string user, string password)
{
    string contact;
    pj_status_t status;
    int ret;
    unsigned int options = 0;
    pjsip_tx_data* tdata = nullptr;
    int mod_id = 0;
    pjsip_media_type ctype;
    pjsip_msg_body* body;
    pj_str_t msg_body;
    pjmedia_sdp_session* sdp_session = nullptr;

    CheckPjThread(callId);
    call->startTime = Tools::Now();
    call->fromUser = from;
    call->toUser = to;
    call->viaIp = viaIp;
    call->contactIp = contactIp;
    call->localSdp = local_sdp;
    ret = create_contact_by_config(from, contactIp, contact);
    if (ret < 0) {
        ERR("error create_uac_contact user:{}\n", from);
        return -1;
    }
    ret = addCall(callId, call);
    if (ret < 0) {
        ERR("call out max exceed, current:{} max:{}\n", mCurrCallCnt, mMaxCall);
        return -1;
    }
    string local_uri = boost::str(boost::format("sip:%s@%s") % from % host);
    string remote_uri = boost::str(boost::format("sip:%s@%s") % to % host);
    string target_uri = boost::str(boost::format("sip:%s@%s") % to % host);
    pj_str_t local_uri_pj = pj_str((char*)local_uri.c_str());
    pj_str_t remote_uri_pj = pj_str((char*)remote_uri.c_str());
    pj_str_t target_uri_pj = pj_str((char*)target_uri.c_str());
    pj_str_t contact_pj = pj_str((char*)contact.c_str());
    /* Create outgoing dialog: */
    status = pjsip_dlg_create_uac(pjsip_ua_instance(), &local_uri_pj, &contact_pj, &remote_uri_pj, &target_uri_pj, &call->dlg);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Dialog creation failed", status);
        ERRLN("pjsip_dlg_create_uac failed, local_uri:{} remote_uri:{} target_uri:{}, contact:{}", local_uri, remote_uri, target_uri, contact);
        return -1;
    }
    pj_strdup2_with_null(call->dlg->pool, &call->dlg->call_id->id, callId.c_str());
    call->callId = (char*)pj_pool_alloc(call->pool, callId.size() + 1);
    std::memcpy(call->callId, callId.c_str(), callId.size());
    call->callId[callId.size()] = '\0';

    pjsip_dlg_inc_lock(call->dlg);
    pjsip_dlg_inc_session(call->dlg, &mEndpoint->mAppModule);
    if (!user.empty()) {
        int credCnt = 1;
        pjsip_cred_info cred;
        cred.realm = pj_str("*");
        cred.scheme = pj_str("digest");
        cred.username = { (char*)user.c_str(), (long)user.size() };
        cred.data_type = 0;
        cred.data = { (char*)password.c_str(), (long)password.size() };
        pjsip_auth_clt_set_credentials(&call->dlg->auth_sess, credCnt, &cred);
    }
    if (viaIp.empty()) {
        viaIp = mEndpoint->mViaIp;
    }
    pjsip_host_port via_addr = { { (char*)viaIp.c_str(), (long)viaIp.size() }, mEndpoint->mPort };
    pjsip_dlg_set_via_sent_by(call->dlg, &via_addr, (pjsip_transport*)mEndpoint->mTransport);
    status = pjmedia_sdp_parse(call->pool, (char*)local_sdp.c_str(), local_sdp.size(), &sdp_session);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "sdp parse error", status);
        ERRLN("invalid sdp:\n{}", local_sdp);
        goto on_error;
    }
    // options |= PJSIP_INV_SUPPORT_100REL;
    // options |= PJSIP_INV_REQUIRE_100REL;
    // options |= PJSIP_INV_SUPPORT_TIMER;
    // options |= PJSIP_INV_REQUIRE_TIMER;
    // options |= PJSIP_INV_ALWAYS_USE_TIMER;
    status = pjsip_inv_create_uac(call->dlg, sdp_session, options, &call->inv);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Invite session creation failed", status);
        goto on_error;
    }
    /* Init Session Timers */
    status = pjsip_timer_init_session(call->inv, &call->timerSetting);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Session Timer init failed", status);
        goto on_error;
    }

    /* Create initial INVITE: */
    status = pjsip_inv_invite(call->inv, &tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to create initial INVITE request", status);
        goto on_error;
    }

    add_sip_hdr(tdata, hdrs);

    // ctype.type = pj_str("application");
    // ctype.subtype = pj_str("sdp");
    // msg_body.ptr = (char*)local_sdp.c_str();
    // msg_body.slen = local_sdp.size();
    // body = pjsip_msg_body_create(tdata->pool, &ctype.type, &ctype.subtype, &msg_body);
    // tdata->msg->body = body;

    /* Send initial INVITE: */
    status = pjsip_inv_send_msg(call->inv, tdata);
    if (status != PJ_SUCCESS) {
        goto on_error;
    }
    pjsip_dlg_add_usage(call->dlg, &mEndpoint->mDlgUsageModule, nullptr);
    mod_id = mEndpoint->mAppModule.id;
    if (call->dlg->mod_data[mod_id] == nullptr) {
        /* In PJSUA2, on_incoming_call() may be called from
         * on_media_transport_created() hence this might already set
         * to allow notification about fail events via on_call_state() and
         * on_call_tsx_state().
         */
        call->dlg->mod_data[mod_id] = call->callId;
        call->dlg->mod_data[mod_id] = call->callId;
    }
    call->timeout = timeout;
    startTimer(call);
    pjsip_dlg_dec_lock(call->dlg);
    return PJ_SUCCESS;

on_error:
    pjsip_dlg_dec_lock(call->dlg);
    return -1;
}

void CallService::timer_cb(pj_timer_heap_t* timer_heap, struct pj_timer_entry* entry)
{
    string callId = (char*)entry->user_data;
    PJ_UNUSED_ARG(timer_heap);
    auto callService = GetInstance();
    callService->timerCb(callId);
}

void CallService::timerCb(string callId)
{
    auto call = getCall(callId);
    if (!call || call->timer.id == 0) {
        return;
    }
    call->timer.id = 0;
    INFO("hangup call when call timeout, timeout:{}s callId:{}\n", call->timeout, call->callId);
    hangup(call, 408, "");
}

void CallService::startTimer(shared_ptr<SipCall> call)
{
    // pj_time_val delay = { 60, 0 };
    // if (call->timeout > 0) {
    //     delay.sec = call->timeout;
    // }
    // pj_timer_entry_init(&call->timer, 1, call->callId, timer_cb);
    // pjsip_endpt_schedule_timer(mEndpoint->mEndpt, &call->timer, &delay);
}

void CallService::stopTimer(shared_ptr<SipCall> call)
{
    if (call->timer.id != 0) {
        pjsip_endpt_cancel_timer(mEndpoint->mEndpt, &call->timer);
        call->timer.id = 0;
    }
}

void CallService::add_sip_hdr(pjsip_tx_data* tdata, map<string, string>& hdrs)
{
    const pj_str_t STR_USER_AGENT = { "User-Agent", 10 };
    pj_str_t user_agent = pj_str("mcu");
    pjsip_hdr* user_agent_hdr = (pjsip_hdr*)pjsip_generic_string_hdr_create(tdata->pool, &STR_USER_AGENT, &user_agent);
    pjsip_msg_add_hdr(tdata->msg, user_agent_hdr);
    for (auto it : hdrs) {
        pj_str_t hdr_name = { (char*)it.first.c_str(), (long)it.first.size() };
        pj_str_t hdr_value = { (char*)it.second.c_str(), (long)it.second.size() };
        pjsip_hdr* hdr = (pjsip_hdr*)pjsip_generic_string_hdr_create(tdata->pool, &hdr_name, &hdr_value);
        pjsip_msg_add_hdr(tdata->msg, hdr);
    }
}

int CallService::early(shared_ptr<SipCall> call, string local_sdp)
{
    int ret = 0;
    pj_status_t status;
    int code = 183;
    string reason = "Session Progress";
    pj_str_t reason_pj = pj_str((char*)reason.c_str());
    pjsip_tx_data* tdata = nullptr;
    pjsip_media_type ctype;
    pjsip_msg_body* body;
    pj_str_t msg_body;

    CheckPjThread(call->callId);
    pjsip_dlg_inc_lock(call->dlg);
    status = pjsip_inv_answer(call->inv, code, &reason_pj, nullptr, &tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error creating response", status);
        ret = -1;
        goto on_return;
    }

    ctype.type = pj_str("application");
    ctype.subtype = pj_str("sdp");
    msg_body.ptr = (char*)local_sdp.c_str();
    msg_body.slen = local_sdp.size();
    body = pjsip_msg_body_create(tdata->pool, &ctype.type, &ctype.subtype, &msg_body);
    tdata->msg->body = body;
    /* Send the message */
    status = pjsip_inv_send_msg(call->inv, tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error sending response", status);
        ret = -1;
        goto on_return;
    }
    pjsip_dlg_dec_lock(call->dlg);
    return 0;
on_return:
    pjsip_dlg_dec_lock(call->dlg);
    return ret;
}

int CallService::answer(shared_ptr<SipCall> call, string local_sdp)
{
    int ret = 0;
    pj_status_t status;
    int code = 200;
    string reason = "OK";
    pj_str_t reason_pj = pj_str((char*)reason.c_str());
    pjsip_tx_data* tdata = nullptr;
    pjsip_media_type ctype;
    pjsip_msg_body* body;
    pj_str_t msg_body;

    CheckPjThread(call->callId);
    pjsip_dlg_inc_lock(call->dlg);
    status = pjsip_inv_answer(call->inv, code, &reason_pj, nullptr, &tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error creating response", status);
        ret = -1;
        goto on_return;
    }

    ctype.type = pj_str("application");
    ctype.subtype = pj_str("sdp");
    msg_body.ptr = (char*)local_sdp.c_str();
    msg_body.slen = local_sdp.size();
    body = pjsip_msg_body_create(tdata->pool, &ctype.type, &ctype.subtype, &msg_body);
    tdata->msg->body = body;
    /* Send the message */
    status = pjsip_inv_send_msg(call->inv, tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error sending response", status);
        ret = -1;
        goto on_return;
    }
    pjsip_dlg_dec_lock(call->dlg);
    return 0;
on_return:
    pjsip_dlg_dec_lock(call->dlg);
    return ret;
}

int CallService::hangup(shared_ptr<SipCall> call, int code, string reason)
{
    return call->hangup(code, reason);
}

int CallService::reinvite(shared_ptr<SipCall> call, string local_sdp)
{
    int ret = 0;
    pj_status_t status;
    string contact;
    pjmedia_sdp_session* new_offer = nullptr;

    CheckPjThread(call->callId);

    ret = create_contact_by_config(call->fromUser, call->contactIp, contact);
    if (ret < 0) {
        ERR("error create_uac_contact user:{} callId:{}\n", call->fromUser, call->callId);
        return -1;
    }
    pjsip_tx_data* tdata = nullptr;
    pj_str_t contact_pj = pj_str((char*)contact.c_str());

    status = pjmedia_sdp_parse(call->pool, (char*)local_sdp.c_str(), local_sdp.size(), &new_offer);
    if (PJ_SUCCESS != status) {
        ERR("parse sdp error when reinvite, sdp:{} callId:{}\n", local_sdp, call->callId);
        return -1;
    }

    pjsip_dlg_inc_lock(call->dlg);
    status = pjsip_inv_reinvite(call->inv, &contact_pj, new_offer, &tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error creating response", status);
        ret = -1;
        goto on_return;
    }
    /* Send the message */
    status = pjsip_inv_send_msg(call->inv, tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error sending response", status);
        ret = -1;
        goto on_return;
    }
    pjsip_dlg_dec_lock(call->dlg);
    return 0;
on_return:
    pjsip_dlg_dec_lock(call->dlg);
    return ret;
}

int CallService::update(shared_ptr<SipCall> call, string local_sdp)
{
    int ret = 0;
    pj_status_t status;
    string contact;
    pjsip_media_type ctype;
    pjsip_msg_body* body;
    pj_str_t msg_body;

    CheckPjThread(call->callId);

    ret = create_contact_by_config(call->fromUser, call->contactIp, contact);
    if (ret < 0) {
        ERR("error create_uac_contact user:{} callId:{}\n", call->fromUser, call->callId);
        return -1;
    }
    pjsip_tx_data* tdata = nullptr;
    pj_str_t contact_pj = pj_str((char*)contact.c_str());

    pjsip_dlg_inc_lock(call->dlg);
    status = pjsip_inv_update(call->inv, &contact_pj, nullptr, &tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error creating response", status);
        ret = -1;
        goto on_return;
    }
    ctype.type = pj_str("application");
    ctype.subtype = pj_str("sdp");
    msg_body.ptr = (char*)local_sdp.c_str();
    msg_body.slen = local_sdp.size();
    body = pjsip_msg_body_create(tdata->pool, &ctype.type, &ctype.subtype, &msg_body);
    tdata->msg->body = body;
    /* Send the message */
    status = pjsip_inv_send_msg(call->inv, tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Error sending response", status);
        ret = -1;
        goto on_return;
    }
    pjsip_dlg_dec_lock(call->dlg);
    return 0;
on_return:
    pjsip_dlg_dec_lock(call->dlg);
    return ret;
}

int CallService::sendMessage(shared_ptr<SipCall> call, string body)
{
    pjsip_dialog* dlg = call->dlg;
    pj_str_t type = pj_str("application");
    pj_str_t subtype = pj_str("json");
    pj_str_t content = pj_str((char*)body.c_str());
    pjsip_tx_data* tdata;
    pj_status_t status;

    CheckPjThread(call->callId);

    pjsip_dlg_inc_lock(dlg);

    /* Create request message. */
    status = pjsip_dlg_create_request(call->inv->dlg, &pjsip_message_method,
        -1, &tdata);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to create MESSAGE request", status);
        goto on_return;
    }

    /* Create "text/plain" message body. */
    tdata->msg->body = pjsip_msg_body_create(tdata->pool, &type, &subtype, &content);
    if (tdata->msg->body == NULL) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to create msg body", PJ_ENOMEM);
        pjsip_tx_data_dec_ref(tdata);
        status = PJ_ENOMEM;
        goto on_return;
    }

    /* Send the request. */
    status = pjsip_dlg_send_request(call->inv->dlg, tdata, -1, nullptr);
    if (status != PJ_SUCCESS) {
        mEndpoint->PjsipPerror(__FILE__, __LINE__, "Unable to send MESSAGE request", status);
        goto on_return;
    }
    pjsip_dlg_dec_lock(dlg);
    return 0;
on_return:
    if (dlg) {
        pjsip_dlg_dec_lock(dlg);
    }
    return -1;
}

int CallService::addCall(string callId, shared_ptr<SipCall> call)
{
    std::lock_guard<mutex> l(mCallMutex);
    if (mCurrCallCnt >= mMaxCall) {
        return -1;
    }
    auto it = mCalls.find(callId);
    if (it != mCalls.end()) {
        return -1;
    }
    ++mCurrCallCnt;
    mCalls.emplace(callId, call);
    return 0;
}

shared_ptr<SipCall> CallService::getCall(string callId)
{
    std::lock_guard<mutex> l(mCallMutex);
    auto it = mCalls.find(callId);
    if (it == mCalls.end()) {
        return nullptr;
    }
    return it->second;
}

void CallService::delCall(string callId)
{
    std::lock_guard<mutex> l(mCallMutex);
    auto it = mCalls.find(callId);
    if (it == mCalls.end()) {
        return;
    }
    --mCurrCallCnt;
    mCalls.erase(it);
}

int CallService::getCallCount()
{
    std::lock_guard<mutex> l(mCallMutex);
    return mCalls.size();
}

void CallService::setCallListener(shared_ptr<CallListener> callListener)
{
    std::lock_guard<mutex> l(mListenerMutex);
    mCallListener = callListener;
}

shared_ptr<CallListener> CallService::getCallListener()
{
    std::lock_guard<mutex> l(mListenerMutex);
    return mCallListener;
}

void CallService::CheckPjThread(string callId)
{
    if (pj_thread_is_registered()) {
        return;
    }
    string thread_name = boost::str(boost::format("call:%s") % callId);
    pj_thread_desc desc;
    pj_bzero(&desc, sizeof(desc));
    pj_thread_t* thread = nullptr;
    pj_thread_register(thread_name.c_str(), desc, &thread);
}
