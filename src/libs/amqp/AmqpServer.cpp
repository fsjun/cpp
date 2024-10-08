
#define THIS_MODULE "AMQP"

#include "AmqpServer.h"
#include "log/Log.h"

int AmqpServer::init(string host, int port, string user, string password, string vhost, string exchange, string queue, string serverId, bool sendonly)
{
    mHost = host;
    mPort = port;
    mUser = user;
    mUserPwd = password;
    mVhost = vhost;
    mExchangeName = exchange;
    mQueueName = queue;
    mServerId = serverId;
    mChannelNumber = 1;
    mSendOnly = sendonly;
    return 0;
}

int AmqpServer::connect()
{
    int ret;
    amqp_rpc_reply_t rpc_reply;

    mConnection = amqp_new_connection();
    mSocket = amqp_tcp_socket_new(mConnection);
    if (!mSocket) {
        ERR("creating TCP socket error");
        return -1;
    }

    ret = amqp_socket_open(mSocket, mHost.c_str(), mPort);
    if (ret) {
        ERR("opening TCP socket error");
        return -1;
    }

    rpc_reply = amqp_login(mConnection, mVhost.c_str(), 0, AMQP_DEFAULT_FRAME_SIZE, 60, AMQP_SASL_METHOD_PLAIN, mUser.c_str(), mUserPwd.c_str());
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        ERR("amqp_login error:{}\n", rpc_reply.reply_type);
        return -1;
    }

    amqp_channel_open(mConnection, mChannelNumber);
    rpc_reply = amqp_get_rpc_reply(mConnection);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        ERR("opening channel error:{}\n", rpc_reply.reply_type);
        return -1;
    }
    ret = sendInit();
    if (ret < 0) {
        return -1;
    }
    ret = receiveInit();
    if (ret < 0) {
        return -1;
    }

    return 0;
}

int AmqpServer::disconnect()
{
    int ret = 0;
    amqp_rpc_reply_t rpc_reply;

    if (mConnection == nullptr) {
        return -1;
    }
    rpc_reply = amqp_channel_close(mConnection, mChannelNumber, AMQP_REPLY_SUCCESS);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        ERR("closing channel error:{}\n", rpc_reply.reply_type);
    }

    rpc_reply = amqp_connection_close(mConnection, AMQP_REPLY_SUCCESS);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        ERR("closing channel error:{}\n", rpc_reply.reply_type);
    }
    ret = amqp_destroy_connection(mConnection);
    if (ret < 0) {
        ERR("ending connection error\n");
    }
    mConnection = NULL;
    mSocket = NULL;
    return 0;
}

int AmqpServer::sendInit()
{
    amqp_rpc_reply_t rpc_reply;

    amqp_exchange_declare(mConnection, mChannelNumber, amqp_cstring_bytes(mExchangeName.c_str()), amqp_cstring_bytes(mExchangeType.c_str()), 0, 0, 0, 0, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(mConnection);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        ERR("declaring exchange error:{}\n", rpc_reply.reply_type);
        return -1;
    }

    return 0;
}

int AmqpServer::receiveInit()
{
    if (mSendOnly) {
        return 0;
    }
    amqp_rpc_reply_t rpc_reply;
    amqp_bytes_t queuenameByte;

    amqp_queue_declare_ok_t* r = amqp_queue_declare(mConnection, mChannelNumber, amqp_cstring_bytes(mQueueName.c_str()), 0, 0, 0, 1, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(mConnection);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        ERR("declaring queue error:{}\n", rpc_reply.reply_type);
        return -1;
    }

    queuenameByte = amqp_bytes_malloc_dup(r->queue);
    if (queuenameByte.bytes == NULL) {
        ERR("out of memory while copying queue name\n");
        return -1;
    }

    amqp_queue_bind(mConnection, mChannelNumber, queuenameByte, amqp_cstring_bytes(mExchangeName.c_str()), amqp_cstring_bytes(mServerId.c_str()), amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(mConnection);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        ERR("binding error:{}\n", rpc_reply.reply_type);
        return -1;
    }

    amqp_basic_consume(mConnection, mChannelNumber, queuenameByte, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(mConnection);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        ERR("consuming error:{}\n", rpc_reply.reply_type);
        return -1;
    }

    DEBUG("the queuename is {:.{}}\n", queuenameByte.bytes, queuenameByte.len);
    amqp_bytes_free(queuenameByte);
    return 0;
}

void* AmqpServer::ReceiveFunc(void* arg)
{
    AmqpServer* s = (AmqpServer*)arg;
    s->receiveFunc();
    return nullptr;
}

int AmqpServer::receiveFunc()
{
    while (true) {
        amqp_rpc_reply_t res;
        amqp_envelope_t envelope;

        // amqp_maybe_release_buffers(mConnection);
        res = amqp_consume_message(mConnection, &envelope, NULL, 0);
        if (AMQP_RESPONSE_NORMAL != res.reply_type) {
            ERR("the res.reply_type={},res.library_error={}\n", res.reply_type, res.library_error);

            if (AMQP_RESPONSE_LIBRARY_EXCEPTION == res.reply_type && (AMQP_STATUS_SOCKET_ERROR == res.library_error || AMQP_STATUS_UNEXPECTED_STATE == res.library_error)) {
                reConnect();
            }
            continue;
        }
        DEBUG("recv from amqp message is {:.{}}\n", (char*)envelope.message.body.bytes, (int)envelope.message.body.len);
        int ret = 0;
        string msg((const char*)envelope.message.body.bytes, envelope.message.body.len);
        for (auto it = mCallbacks.begin(); it != mCallbacks.end(); it++) {
            auto func = *it;
            ret = func(msg);
            if (ret == 0) {
                break;
            }
        }
        amqp_destroy_envelope(&envelope);

        // std::this_thread::yield();
        //  std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

int AmqpServer::addCallback(function<int(string)>& func)
{
    mCallbacks.push_back(func);
    return 0;
}

string AmqpServer::getServerId()
{
    return mServerId;
}

void AmqpServer::reConnect()
{
    disconnect();
    connect();
}

int AmqpServer::send(string routingKey, map<string, any>& root)
{
    string sendString;
    if (root.empty()) {
        ERR("the rootInfo is empty for routingKey:{}\n", routingKey);
        return -1;
    }
    int ret = mJsoncpp.jsonToString(root, sendString);
    if (sendString.empty()) {
        ERR("jsoncpp gen error ret[{}]", ret);
        return -1;
    }
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */

    std::lock_guard<std::mutex> l(mConnLock);
    if (!mConnection) {
        ERR("publishing error:connection to amqp is NULL\n");
        return -1;
    }
    ret = amqp_basic_publish(mConnection, mChannelNumber, amqp_cstring_bytes(mExchangeName.c_str()), amqp_cstring_bytes(routingKey.c_str()), 0, 0, &props, amqp_cstring_bytes(sendString.c_str()));
    if (ret < 0) {
        ERR("publishing msg error:{}\n", routingKey);
        return -1;
    }
    INFO("send ok exchange[{}] routingKey=[{}] msg=[{}]\n", mExchangeName.c_str(), routingKey.c_str(), sendString.c_str());
    return 0;
}
