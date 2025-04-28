#include "Amqpcpp.h"
#include "amqpcpp/reliable.h"
#include "log/Log.h"
#include "magic_enum_all.hpp"
#include <ev.h>
#include <mutex>
#include <thread>

Amqpcpp::~Amqpcpp()
{
}

int Amqpcpp::init(struct options opt, bool sendOnly)
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
    mSendOnly = sendOnly;
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

void Amqpcpp::AsyncCallback(EV_P_ ev_async*, int)
{
    ev_break(loop, EVBREAK_ALL);
}

void Amqpcpp::stop()
{
    mIsRun = false;
    // struct ev_loop* loop = EV_DEFAULT;
    // ev_async_init(&mAsyncWatcher, AsyncCallback);
    // ev_async_start(loop, &mAsyncWatcher);
    // ev_async_send(loop, &mAsyncWatcher);
}

void Amqpcpp::onMessage(const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
{
    mReceiveChannel->ack(deliveryTag);
    string msg(message.body(), message.bodySize());
    if (mCallback) {
        mCallback(msg);
    }
}

void Amqpcpp::threadFunc()
{
    struct ev_loop* loop = EV_DEFAULT;
    int loopCount = 0;
    while (mIsRun || loopCount == 0) {
        ++loopCount;
        disconnect();
        connect();
        // run the loop
        ev_run(loop, 0);
        if (mIsRun) {
            INFOLN("ev_run return count:{} reconnect after 2s", loopCount);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    disconnect();
}

int Amqpcpp::connect()
{
    auto weak = weak_from_this();
    struct ev_loop* loop = EV_DEFAULT;
    mHandler = make_shared<AMQP::LibEvHandler>(loop);
    AMQP::Address address(std::format("amqp://{}:{}@{}:{}/{}", mUser, mUserPwd, mHost, mPort, mVhost));
    mConnection = make_shared<AMQP::TcpConnection>(mHandler.get(), address);
    mReceiveChannel = make_shared<AMQP::TcpChannel>(mConnection.get());
    mSendChannel = make_shared<AMQP::TcpChannel>(mConnection.get());
    mReliable = make_shared<AMQP::Reliable<>>(*mSendChannel);
    mReceiveChannel->onError([](const char* message) {
        ERRLN("channel error: {}", message);
    });
    mReceiveChannel->onReady([weak]() {
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
    if (mSendOnly) {
        return;
    }
    auto weak = weak_from_this();
    mReceiveChannel->declareExchange(mExchangeName, AMQP::direct, AMQP::durable);
    mReceiveChannel->declareQueue(mQueueName, AMQP::durable + AMQP::autodelete);
    mReceiveChannel->bindQueue(mExchangeName, mQueueName, mBindingKey);

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
    mReceiveChannel->consume(mQueueName)
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
        doSend(routingKey, msg);
    }
    mSendList.clear();
}

int Amqpcpp::disconnect()
{
    std::lock_guard l(mMutex);
    mIsReady = false;
    if (mReceiveChannel) {
        mReceiveChannel->close();
    }
    if (mSendChannel) {
        mSendChannel->close();
    }
    if (mConnection) {
        mConnection->close(true);
    }
    mConnection.reset();
    mReceiveChannel.reset();
    mSendChannel.reset();
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
    if (mIsReady && mSendChannel) {
        doSend(routingKey, msg);
    } else {
        mSendList.emplace_back(routingKey, msg);
    }
    return 0;
}

int Amqpcpp::doSend(string routingKey, string msg)
{
    // mChannel->publish(mExchangeName, routingKey, msg);
    mReliable->publish(mExchangeName, routingKey, msg)
        .onAck([this, routingKey, msg]() {
            // the message has been acknowledged by RabbitMQ (in your application
            // code you can now safely discard the message as it has been picked up)
            INFOLN("write ok, exchangeName:{} routingKey:{} msg:{}", mExchangeName, routingKey, msg);
            internalBreak();
        })
        .onNack([this, routingKey, msg]() {
            // the message has _explicitly_ been nack'ed by RabbitMQ (in your application
            // code you probably want to log or handle this to avoid data-loss)
            ERRLN("onNack, exchangeName:{} routingKey:{} msg:{}", mExchangeName, routingKey, msg);
            internalBreak();
        })
        .onLost([this, routingKey, msg]() {
            // because the implementation for onNack() and onError() will be the same
            // in many applications, you can also choose to install a onLost() handler,
            // which is called when the message has either been nack'ed, or lost.
            ERRLN("onLost, exchangeName:{} routingKey:{} msg:{}", mExchangeName, routingKey, msg);
            internalBreak();
        })
        .onError([this, routingKey, msg](const char* message) {
            // a channel-error occurred before any ack or nack was received, and the
            // message is probably lost (which you might want to handle)
            ERRLN("onLost, exchangeName:{} routingKey:{} msg:{} err:{}", mExchangeName, routingKey, msg, message);
            internalBreak();
        });
    return 0;
}

void Amqpcpp::internalBreak()
{
    if (!mIsRun) {
        struct ev_loop* loop = EV_DEFAULT;
        ev_break(loop, EVBREAK_ALL);
    }
}

void Amqpcpp::join()
{
    if (mThread) {
        mThread->join();
    }
}