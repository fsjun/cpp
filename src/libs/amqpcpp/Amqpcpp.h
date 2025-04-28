#pragma once

#include "jsoncc/Jsoncc.h"
#include "threadpool/Thread.h"
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>
#include <ev.h>
#include <amqpcpp/libev.h>
#include <functional>
#include <memory>
#include <string>

using std::function;
using std::string;

class Amqpcpp : public std::enable_shared_from_this<Amqpcpp> {
public:
    struct options {
        string host;
        int port = 0;
        string user;
        string password;
        string vhost;
        string exchange;
        string queue;
        string bindingkey;
        string routingkey;
    };
    ~Amqpcpp();
    int init(struct options opt, bool sendOnly = false);
    int setCallback(function<int(string)> func);
    int start();
    void stop();
    int send(string routingKey, Json::Value& root);
    int send(string routingKey, string msg);
    void join();

private:
    void onMessage(const AMQP::Message& message, uint64_t deliveryTag, bool redelivered);
    void threadFunc();
    int connect();
    int disconnect();
    void onReady();
    void sendPendingList();
    static void AsyncCallback(EV_P_ ev_async*, int);
    int doSend(string routingKey, string msg);
    void internalBreak();

private:
    bool mIsRun = true;
    std::mutex mMutex;
    bool mIsReady = false;
    std::list<std::pair<string, string>> mSendList;
    shared_ptr<AMQP::TcpConnection> mConnection;
    shared_ptr<AMQP::TcpChannel> mReceiveChannel;
    shared_ptr<AMQP::TcpChannel> mSendChannel;
    shared_ptr<AMQP::LibEvHandler> mHandler;
    shared_ptr<AMQP::Reliable<>> mReliable;
    int mSendCount = 0;

    string mBindingKey;
    string mRoutingKey;
    int mChannelNumber;
    string mExchangeName;
    string mQueueName;
    string mHost;
    string mVhost;
    int mPort;
    string mUser;
    string mUserPwd;
    bool mSendOnly = false;

    function<int(string)> mCallback;
    shared_ptr<Thread> mThread;
    ev_async mAsyncWatcher;
};