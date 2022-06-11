#include "SipCall.h"
#include "sipserver/CallService.h"

SipCall::SipCall()
{
    auto endpoint = Endpoint::GetInstance();
    timerSetting.min_se = 90;
    timerSetting.sess_expires = 1800;
    pool = endpoint->genPool("call");
    memset(&timer, 0, sizeof(timer));
}

SipCall::~SipCall()
{
    CallService::CheckPjThread(callId ? callId : "");
    auto endpoint = Endpoint::GetInstance();
    if (inv) {
        inv = nullptr;
    }
    if (dlg) {
        pjsip_dlg_dec_session(dlg, &endpoint->mAppModule);
        dlg = nullptr;
    }
    if (pool) {
        pj_pool_release(pool);
        pool = nullptr;
    }
}

int SipCall::hangup(int code, string reason)
{
    pj_status_t status;
    pj_str_t reasonstr = { (char*)reason.c_str(), (long)reason.size() };
    pjsip_tx_data* tdata = nullptr;
    auto callService = CallService::GetInstance();

    callService->CheckPjThread(callId);
    pjsip_dlg_inc_lock(dlg);
    status = pjsip_inv_end_session(inv, code, &reasonstr, &tdata);
    if (status != PJ_SUCCESS) {
        Endpoint::PjsipPerror(__FILE__, __LINE__, "Failed to create end session message", status);
        goto on_return;
    }

    if (tdata) {
        /* Send the message */
        status = pjsip_inv_send_msg(inv, tdata);
        if (status != PJ_SUCCESS) {
            Endpoint::PjsipPerror(__FILE__, __LINE__, "Error sending response", status);
            goto on_return;
        }
    }
    pjsip_dlg_dec_lock(dlg);
    return 0;
on_return:
    pjsip_dlg_dec_lock(dlg);
    return -1;
}
