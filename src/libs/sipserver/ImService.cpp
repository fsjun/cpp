#include "ImService.h"
#include "Endpoint.h"

void ImService::setImListener(shared_ptr<ImListener> imListener)
{
    mImListener = imListener;
}

shared_ptr<ImListener> ImService::getImListener()
{
    return mImListener;
}

int ImService::sendMessage(string from, string to, string callId, string target_addr, string body)
{
    auto endpoint = Endpoint::GetInstance();
    pjsip_tx_data* tdata;
    const pj_str_t mime_text_plain = pj_str("text/plain");
    pjsip_media_type media_type;
    pj_status_t status;
    const pjsip_method pjsip_message_method = {
        (pjsip_method_e)PJSIP_OTHER_METHOD,
        { "MESSAGE", 7 }
    };
    pj_str_t type = pj_str("application");
    pj_str_t subtype = pj_str("json");

    CheckPjThread();
    string target_uri = boost::str(boost::format("sip:%s") % target_addr);
    pj_str_t target_uri_pj = pj_str((char*)target_uri.c_str());
    string from_uri = boost::str(boost::format("sip:%s@%s") % from % target_addr);
    pj_str_t from_uri_pj = pj_str((char*)from_uri.c_str());
    string to_uri = boost::str(boost::format("sip:%s@%s") % to % target_addr);
    pj_str_t to_uri_pj = pj_str((char*)to_uri.c_str());
    pj_str_t callid_pj = pj_str((char*)callId.c_str());
    string host = endpoint->getContactIp();
    int port = endpoint->getPort();
    string contact = boost::str(boost::format("<sip:%s@%s:%d>") % from % host % port);
    pj_str_t contact_pj = pj_str((char*)contact.c_str());
    pj_str_t text = pj_str((char*)body.c_str());
    pjsip_host_port via_addr = { { (char*)endpoint->mViaIp.c_str(), (long)endpoint->mViaIp.size() }, endpoint->mPort };

    /* Create request. */
    status = pjsip_endpt_create_request(endpoint->mEndpt, &pjsip_message_method, &target_uri_pj, &from_uri_pj, &to_uri_pj, &contact_pj, &callid_pj, -1, &text, &tdata);
    if (status != PJ_SUCCESS) {
        endpoint->PjsipPerror(__FILE__, __LINE__, "Unable to create request", status);
        return status;
    }
    if (false) {
        pjsip_tpselector tp_sel;
        tp_sel.type = PJSIP_TPSELECTOR_TRANSPORT;
        tp_sel.u.transport = endpoint->mTransport;
        pjsip_tx_data_set_transport(tdata, &tp_sel);
    }

    /* Create suitable Contact header unless a Contact header has been
     * set in the account.
     */
    /* Ticket #1632: According to RFC 3428:
     * MESSAGE requests do not initiate dialogs.
     * User Agents MUST NOT insert Contact header fields into MESSAGE requests
     */
    /*
    if (acc->contact.slen) {
        contact = acc->contact;
    } else {
        status = pjsua_acc_create_uac_contact(tdata->pool, &contact, acc_id, to);
        if (status != PJ_SUCCESS) {
            pjsua_perror(THIS_FILE, "Unable to generate Contact header", status);
            pjsip_tx_data_dec_ref(tdata);
            return status;
        }
    }

    pjsip_msg_add_hdr( tdata->msg, (pjsip_hdr*)
        pjsip_generic_string_hdr_create(tdata->pool,
                                        &STR_CONTACT, &contact));
    */

    /* Add message body */
    //    tdata->msg->body = pjsip_msg_body_create(tdata->pool, &type,
    //        &subtype, &);
    //    if (tdata->msg->body == NULL) {
    //        pjsua_perror(THIS_FILE, "Unable to create msg body", PJ_ENOMEM);
    //        pjsip_tx_data_dec_ref(tdata);
    //        return PJ_ENOMEM;
    //    }

    tdata->via_addr = via_addr;

    string* pCallId = new string(callId);
    /* Send request (statefully) */
    status = pjsip_endpt_send_request(endpoint->mEndpt, tdata, -1, (void*)pCallId, &im_callback);
    if (status != PJ_SUCCESS) {
        endpoint->PjsipPerror(__FILE__, __LINE__, "Unable to send request", status);
        return status;
    }
    return PJ_SUCCESS;
}

int ImService::sendOptions(string from, string to, string callId, string target_addr)
{
    auto endpoint = Endpoint::GetInstance();
    pjsip_tx_data* tdata;
    pj_status_t status;
    const pjsip_method pjsip_options_method = {
        (pjsip_method_e)PJSIP_OPTIONS_METHOD,
        { "OPTIONS", 7 }
    };

    CheckPjThread();
    string target_uri = boost::str(boost::format("sip:%s") % target_addr);
    pj_str_t target_uri_pj = pj_str((char*)target_uri.c_str());
    string from_uri = boost::str(boost::format("sip:%s@%s") % from % target_addr);
    pj_str_t from_uri_pj = pj_str((char*)from_uri.c_str());
    string to_uri = boost::str(boost::format("sip:%s@%s") % to % target_addr);
    pj_str_t to_uri_pj = pj_str((char*)to_uri.c_str());
    pj_str_t callid_pj = pj_str((char*)callId.c_str());
    string host = endpoint->getContactIp();
    int port = endpoint->getPort();
    string contact = boost::str(boost::format("<sip:%s@%s:%d>") % from % host % port);
    pj_str_t contact_pj = pj_str((char*)contact.c_str());
    pjsip_host_port via_addr = { { (char*)endpoint->mViaIp.c_str(), (long)endpoint->mViaIp.size() }, endpoint->mPort };

    /* Create request. */
    status = pjsip_endpt_create_request(endpoint->mEndpt, &pjsip_options_method, &target_uri_pj, &from_uri_pj, &to_uri_pj, &contact_pj, &callid_pj, -1, nullptr, &tdata);
    if (status != PJ_SUCCESS) {
        endpoint->PjsipPerror(__FILE__, __LINE__, "Unable to create request", status);
        return status;
    }
    if (false) {
        pjsip_tpselector tp_sel;
        tp_sel.type = PJSIP_TPSELECTOR_TRANSPORT;
        tp_sel.u.transport = endpoint->mTransport;
        pjsip_tx_data_set_transport(tdata, &tp_sel);
    }

    /* Create suitable Contact header unless a Contact header has been
     * set in the account.
     */
    /* Ticket #1632: According to RFC 3428:
     * MESSAGE requests do not initiate dialogs.
     * User Agents MUST NOT insert Contact header fields into MESSAGE requests
     */
    /*
    if (acc->contact.slen) {
        contact = acc->contact;
    } else {
        status = pjsua_acc_create_uac_contact(tdata->pool, &contact, acc_id, to);
        if (status != PJ_SUCCESS) {
            pjsua_perror(THIS_FILE, "Unable to generate Contact header", status);
            pjsip_tx_data_dec_ref(tdata);
            return status;
        }
    }

    pjsip_msg_add_hdr( tdata->msg, (pjsip_hdr*)
        pjsip_generic_string_hdr_create(tdata->pool,
                                        &STR_CONTACT, &contact));
    */

    /* Add message body */
    //    tdata->msg->body = pjsip_msg_body_create(tdata->pool, &type,
    //        &subtype, &);
    //    if (tdata->msg->body == NULL) {
    //        pjsua_perror(THIS_FILE, "Unable to create msg body", PJ_ENOMEM);
    //        pjsip_tx_data_dec_ref(tdata);
    //        return PJ_ENOMEM;
    //    }

    tdata->via_addr = via_addr;

    string* pCallId = new string(callId);
    /* Send request (statefully) */
    status = pjsip_endpt_send_request(endpoint->mEndpt, tdata, -1, (void*)pCallId, &im_callback);
    if (status != PJ_SUCCESS) {
        endpoint->PjsipPerror(__FILE__, __LINE__, "Unable to send request", status);
        return status;
    }
    return PJ_SUCCESS;
}

void ImService::im_callback(void* token, pjsip_event* e)
{
    auto imService = ImService::GetInstance();
    imService->imCallback(token, e);
}

void ImService::imCallback(void* token, pjsip_event* e)
{
    string* pCallId = (string*)token;
    string callId = *pCallId;
    delete pCallId;
    if (e->type == PJSIP_EVENT_TSX_STATE) {
        pjsip_transaction* tsx = e->body.tsx_state.tsx;
        int code = tsx->status_code;
        string reason(tsx->status_text.ptr, tsx->status_text.slen);
        string body;
        if (e->body.tsx_state.type == PJSIP_EVENT_RX_MSG) {
            pjsip_rx_data* rdata = e->body.tsx_state.src.rdata;
            if (rdata && rdata->msg_info.msg->body) {
                body = string((char*)rdata->msg_info.msg->body->data, rdata->msg_info.msg->body->len);
            }
        }
        INFOLN("receive response callId:{} code:{} reason:{} body:{}\n", callId, code, reason, body);
        auto imListener = getImListener();
        if (imListener) {
            imListener->on_rx_response(callId, code, reason, body);
        }
    }
}

int ImService::on_message(pjsip_rx_data* rdata)
{
    auto endpoint = Endpoint::GetInstance();
    pjsip_tx_data* tdata = nullptr;
    /* Should not have any transaction attached to rdata. */
    PJ_ASSERT_RETURN(pjsip_rdata_get_tsx(rdata) == NULL, PJ_FALSE);
    /* Should not have any dialog attached to rdata. */
    PJ_ASSERT_RETURN(pjsip_rdata_get_dlg(rdata) == NULL, PJ_FALSE);

    /* Respond with 200 first, so that remote doesn't retransmit in case
     * the UI takes too long to process the message.
     */
    pjsip_endpt_respond(endpoint->mEndpt, NULL, rdata, 200, NULL, NULL, NULL, NULL);
    string method(rdata->msg_info.msg->line.req.method.name.ptr, rdata->msg_info.msg->line.req.method.name.slen);
    string callId = string(rdata->msg_info.cid->id.ptr, rdata->msg_info.cid->id.slen);
    string body = string((char*)rdata->msg_info.msg->body->data, rdata->msg_info.msg->body->len);
    INFOLN("receive request callId:{} method:{} body:{}\n", callId, method, body);
    auto imListener = getImListener();
    if (imListener) {
        imListener->on_rx_request(callId, method, body);
    }
    return PJ_TRUE;
}

int ImService::on_info(pjsip_rx_data* rdata)
{
    auto endpoint = Endpoint::GetInstance();
    pjsip_tx_data* tdata = nullptr;
    /* Should not have any transaction attached to rdata. */
    PJ_ASSERT_RETURN(pjsip_rdata_get_tsx(rdata) == NULL, PJ_FALSE);
    /* Should not have any dialog attached to rdata. */
    PJ_ASSERT_RETURN(pjsip_rdata_get_dlg(rdata) == NULL, PJ_FALSE);

    /* Respond with 200 first, so that remote doesn't retransmit in case
     * the UI takes too long to process the message.
     */
    pjsip_endpt_respond(endpoint->mEndpt, NULL, rdata, 200, NULL, NULL, NULL, NULL);
    string method(rdata->msg_info.msg->line.req.method.name.ptr, rdata->msg_info.msg->line.req.method.name.slen);
    string callId = string(rdata->msg_info.cid->id.ptr, rdata->msg_info.cid->id.slen);
    string body = string((char*)rdata->msg_info.msg->body->data, rdata->msg_info.msg->body->len);
    INFOLN("receive request callId:{} method:{} body:{}\n", callId, method, body);
    auto imListener = getImListener();
    if (imListener) {
        imListener->on_rx_request(callId, method, body);
    }
    return PJ_TRUE;
}

void ImService::CheckPjThread()
{
    if (!pj_thread_is_registered()) {
        string thread_name = boost::str(boost::format("imService"));
        pj_thread_desc desc;
        pj_bzero(&desc, sizeof(desc));
        pj_thread_t* thread = nullptr;
        pj_thread_register(thread_name.c_str(), desc, &thread);
    }
}
