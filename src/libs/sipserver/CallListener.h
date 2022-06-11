#pragma once

#include "sipserver/SipCall.h"

class CallListener {
public:
    virtual void onIncomingCall(shared_ptr<SipCall> call) = 0;
    virtual void onEarly(shared_ptr<SipCall> call) = 0;
    virtual void onConnecting(shared_ptr<SipCall> call) = 0;
    virtual void onConfirmed(shared_ptr<SipCall> call) = 0;
    virtual void onDisconnected(shared_ptr<SipCall> call) = 0;
    virtual void onMessage(shared_ptr<SipCall> call, string body) = 0;
    virtual void onInfo(shared_ptr<SipCall> call, string body) = 0;
    virtual void onRxOffer(shared_ptr<SipCall> call, string remoteSdp, string& localSdp) = 0;
};
