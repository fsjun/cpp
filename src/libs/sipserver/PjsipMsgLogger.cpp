
#include "PjsipMsgLogger.h"

#define THIS_FILE "PjsipMsgLogger.cpp"

bool PjsipMsgLogger::logOptions = false;

pjsip_module PjsipMsgLogger::pjsip_msg_logger = {
    NULL, NULL, /* prev, next.		*/
    { "mod-pjsip-log", 13 }, /* Name.		*/
    -1, /* Id			*/
    PJSIP_MOD_PRIORITY_TRANSPORT_LAYER - 1, /* Priority	        */
    NULL, /* load()		*/
    NULL, /* start()		*/
    NULL, /* stop()		*/
    NULL, /* unload()		*/
    &logging_on_rx_msg, /* on_rx_request()	*/
    &logging_on_rx_msg, /* on_rx_response()	*/
    &logging_on_tx_msg, /* on_tx_request.	*/
    &logging_on_tx_msg, /* on_tx_response()	*/
    NULL, /* on_tsx_state()	*/
};

pj_bool_t PjsipMsgLogger::logging_on_rx_msg(pjsip_rx_data* rdata)
{
    if (!logOptions && !pj_strcmp2(&rdata->msg_info.cseq->method.name, "OPTIONS")) {
        return PJ_FALSE;
    }
    char addr[PJ_INET6_ADDRSTRLEN + 10];
    pj_str_t input_str = pj_str(rdata->pkt_info.src_name);

    INFOLN("RX {} bytes {} from {} {}:"
         "{:.{}}\n"
         "--end msg--\n",
        rdata->msg_info.len, pjsip_rx_data_get_info(rdata), rdata->tp_info.transport->type_name, pj_addr_str_print(&input_str, rdata->pkt_info.src_port, addr, sizeof(addr), 1), rdata->msg_info.msg_buf, (int)rdata->msg_info.len);

    /* Always return false, otherwise messages will not get processed! */
    return PJ_FALSE;
}

/* Notification on outgoing messages */
pj_status_t PjsipMsgLogger::logging_on_tx_msg(pjsip_tx_data* tdata)
{
    pjsip_cseq_hdr* cseq = PJSIP_MSG_CSEQ_HDR(tdata->msg);
    if (!logOptions && !pj_strcmp2(&cseq->method.name, "OPTIONS")) {
        return PJ_SUCCESS;
    }
    char addr[PJ_INET6_ADDRSTRLEN + 10];
    pj_str_t input_str = pj_str(tdata->tp_info.dst_name);

    /* Important note:
     *	tp_info field is only valid after outgoing messages has passed
     *	transport layer. So don't try to access tp_info when the module
     *	has lower priority than transport layer.
     */
    INFOLN("TX {} bytes {} to {} {}:"
         "{:.{}}\n"
         "--end msg--\n",
        (tdata->buf.cur - tdata->buf.start), pjsip_tx_data_get_info(tdata), tdata->tp_info.transport->type_name, pj_addr_str_print(&input_str, tdata->tp_info.dst_port, addr, sizeof(addr), 1), tdata->buf.start, (int)(tdata->buf.cur - tdata->buf.start));

    /* Always return success, otherwise message will not get sent! */
    return PJ_SUCCESS;
}
