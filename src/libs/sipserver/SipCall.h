#pragma once

#include "Endpoint.h"
#include "sipserver/CallDirection.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"

struct SipCall {
    string cname; /** Generate per-session RTCP CNAME, according to RFC 7022. */
    long startTime = 0;
    long ringTime = 0;
    long answerTime = 0;
    long hangupTime = 0;
    pjsip_timer_setting timerSetting;
    char* callId = nullptr;
    CallDirection callDirection = CALL_DIRECTION_OUTBOUND;

    int timeout = 0;
    pj_timer_entry timer;
    pjsip_dialog* dlg = nullptr;
    pjsip_inv_session* inv = nullptr;
    pj_pool_t* pool = nullptr;

    string viaIp;
    string contactIp;
    string localSdp;
    string remoteSdp;
    pjmedia_sdp_session* sdp_session = nullptr;

    string fromUser;
    string toUser;
    map<string, string> hdrs;
    int code;
    string reason;

    string sourceIp;
    int sourcePort;

    SipCall();
    virtual ~SipCall();
    int hangup(int code, string reason);
};
