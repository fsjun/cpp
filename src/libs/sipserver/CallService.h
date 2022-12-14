#pragma once

#include "tools/boost_common.h"
#include "tools/cpp_common.h"

#include "Endpoint.h"
#include "sipserver/CallListener.h"
#include "sipserver/SipCall.h"

class CallService : public Singleton<CallService>, public std::enable_shared_from_this<CallService> {
public:
    CallService();
    void on_incoming_call(pjsip_rx_data* rdata, pjsip_module* mod);
    void on_rx_offer2(pjsip_inv_session* inv, struct pjsip_inv_on_rx_offer_cb_param* param);
    void on_state_changed(pjsip_inv_session* inv, pjsip_event* e);
    void on_message(pjsip_rx_data* rdata);
    void on_info(pjsip_rx_data* rdata);
    int create_contact_by_config(string user, string contactIp, string& contact);
    int create_contact_by_rdate(pj_pool_t* pool, pj_str_t* contact, pjsip_rx_data* rdata);

    int makeCall(shared_ptr<SipCall> call, string callId, string from, string to, string host, int timeout, string viaIp, string contactIp, string local_sdp, map<string, string>& hdrs, string user, string password);
    int early(shared_ptr<SipCall> call, string local_sdp);
    int answer(shared_ptr<SipCall> call, string local_sdp);
    int hangup(shared_ptr<SipCall> call, int code, string reason);
    int reinvite(shared_ptr<SipCall> call, string local_sdp);
    int update(shared_ptr<SipCall> call, string local_sdp);
    int sendMessage(shared_ptr<SipCall> call, string body);

    int addCall(string callId, shared_ptr<SipCall> call);
    shared_ptr<SipCall> getCall(string callId);
    void delCall(string callId);
    int getCallCount();
    void setCallListener(shared_ptr<CallListener> callListener);
    shared_ptr<CallListener> getCallListener();
    static void CheckPjThread(string callId);

    static const pjsip_method pjsip_message_method;

private:
    void fix_contact_hdr(pjsip_rx_data* rdata);
    void add_sip_hdr(pjsip_tx_data* tdata, map<string, string>& hdrs);
    string getFromUser(pjsip_rx_data* rdata);
    string getToUser(pjsip_rx_data* rdata);
    static void timer_cb(pj_timer_heap_t* timer_heap, struct pj_timer_entry* entry);
    void timerCb(string callId);
    void startTimer(shared_ptr<SipCall> call);
    void stopTimer(shared_ptr<SipCall> call);

private:
    mutex mCallMutex;
    map<string, shared_ptr<SipCall>> mCalls;
    int mMaxCall = 1000;
    int mCurrCallCnt = 0;
    mutex mListenerMutex;
    shared_ptr<CallListener> mCallListener;
    shared_ptr<Endpoint> mEndpoint;
};
