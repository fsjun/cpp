#pragma once

#include "Endpoint.h"

class PjsipMsgLogger {
public:
    static pj_bool_t logging_on_rx_msg(pjsip_rx_data* rdata);
    static pj_status_t logging_on_tx_msg(pjsip_tx_data* tdata);

    static pjsip_module pjsip_msg_logger;
    static bool logOptions;
};
