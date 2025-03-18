#pragma once

#include "Endpoint.h"
#include "ImListener.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"

class ImService : public Singleton<ImService>, public std::enable_shared_from_this<ImService> {
public:
    void setImListener(shared_ptr<ImListener> imListener);
    shared_ptr<ImListener> getImListener();
    int sendMessage(string from, string to, string callId, string target_addr, string body);
    int sendOptions(string from, string to, string callId, string target_addr);

    int on_message(pjsip_rx_data* rdata);
    int on_info(pjsip_rx_data* rdata);

    static void CheckPjThread();

private:
    static void im_callback(void* token, pjsip_event* e);
    void imCallback(void* token, pjsip_event* e);

    shared_ptr<ImListener> mImListener;
};
