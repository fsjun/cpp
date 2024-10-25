#include "Amqpcpp.h"
#include "log/Log.h"
#include "magic_enum_all.hpp"
#include <mutex>
#include <thread>

Amqpcpp::~Amqpcpp()
{
}

int Amqpcpp::init(struct options opt)
{
    mHost = opt.host;
    mPort = opt.port;
    mUser = opt.user;
    mUserPwd = opt.password;
    mVhost = opt.vhost;
    mExchangeName = opt.exchange;
    mQueueName = opt.queue;
    mBindingKey = opt.bindingkey;
    mRoutingKey = opt.routingkey;
    mChannelNumber = 1;
    return 0;
}

int Amqpcpp::start()
{
    mIsRun = true;
    auto weak = weak_from_this();
    mThread = make_shared<Thread>();
    mThread->start([weak]() {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        self->threadFunc();
    });
    return 0;
}

void Amqpcpp::stop()
{
    mIsRun = false;
}

void Amqpcpp::onMessage(const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
{
    mChannel->ack(deliveryTag);
    string msg(message.body(), message.bodySize());
    if (mCallback) {
        mCallback(msg);
    }
}

void Amqpcpp::threadFunc()
{
    struct ev_loop* loop = EV_DEFAULT;
    while (mIsRun) {
        disconnect();
        connect();
        // run the loop
        ev_run(loop, 0);
        INFOLN("ev_run return reconnect after 2s");
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int Amqpcpp::connect()
{
    auto weak = weak_from_this();
    struct ev_loop* loop = EV_DEFAULT;
    mHandler = make_shared<AMQP::LibEvHandler>(loop);
    AMQP::Address address(std::format("amqp://{}:{}@{}:{}/{}", mUser, mUserPwd, mHost, mPort, mVhost));
    mConnection = make_shared<AMQP::TcpConnection>(mHandler.get(), address);
    mChannel = make_shared<AMQP::TcpChannel>(mConnection.get());
    mChannel->onError([](const char* message) {
        ERRLN("channel error: {}", message);
    });
    mChannel->onReady([weak]() {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        INFOLN("channel ready");
        self->onReady();
        self->sendPendingList();
    });
    return 0;
}

void Amqpcpp::onReady()
{
    auto weak = weak_from_this();
    mChannel->declareExchange(mExchangeName, AMQP::direct, AMQP::durable);
    mChannel->declareQueue(mQueueName, AMQP::durable + AMQP::autodelete);
    mChannel->bindQueue(mExchangeName, mQueueName, mBindingKey);

    // callback function that is called when the consume operation starts
    auto startCb = [](const std::string& consumertag) {
        INFOLN("consume operation started");
    };

    // callback function that is called when the consume operation failed
    auto errorCb = [](const char* message) {
        ERRLN("consume operation failed, message:{}", message);
    };

    // callback operation when a message was received
    auto messageCb = [weak](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered) {
        auto self = weak.lock();
        if (!self) {
            return;
        }
        self->onMessage(message, deliveryTag, redelivered);
    };

    // callback that is called when the consumer is cancelled by RabbitMQ (this only happens in
    // rare situations, for example when someone removes the queue that you are consuming from)
    auto cancelledCb = [](const std::string& consumertag) {
        INFOLN("consume operation cancelled by the RabbitMQ server");
    };

    // start consuming from the queue, and install the callbacks
    mChannel->consume(mQueueName)
        .onReceived(messageCb)
        .onSuccess(startCb)
        .onCancelled(cancelledCb)
        .onError(errorCb);
}

void Amqpcpp::sendPendingList()
{
    std::lock_guard l(mMutex);
    mIsReady = true;
    for (auto [routingKey, msg] : mSendList) {
        mChannel->publish(mExchangeName, routingKey, msg);
    }
    mSendList.clear();
}

int Amqpcpp::disconnect()
{
    std::lock_guard l(mMutex);
    mIsReady = false;
    if (mChannel) {
        mChannel->close();
    }
    if (mConnection) {
        mConnection->close(true);
    }
    mConnection.reset();
    mChannel.reset();
    mHandler.reset();
    return 0;
}

int Amqpcpp::setCallback(function<int(string)> func)
{
    mCallback = func;
    return 0;
}

int Amqpcpp::send(string routingKey, Json::Value& root)
{
    if (root.empty()) {
        ERR("the content of send is empty for routingKey:{}\n", routingKey);
        return -1;
    }
    string sendString = Jsoncc::JsonToString(root);
    if (sendString.empty()) {
        ERR("jsoncpp gen error");
        return -1;
    }
    return send(routingKey, sendString);
}

int Amqpcpp::send(string routingKey, string msg)
{
    std::lock_guard<std::mutex> l(mMutex);
    INFO("send ok exchange:{} routingKey:{} msg:{}\n", mExchangeName.c_str(), routingKey.c_str(), msg.c_str());
    if (mIsReady && mChannel) {
        mChannel->publish(mExchangeName, routingKey, msg);
    } else {
        mSendList.emplace_back(routingKey, msg);
    }
    return 0;
}

void Amqpcpp::join()
{
    if (mThread) {
        mThread->join();
    }
}