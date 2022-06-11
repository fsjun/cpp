#pragma once

#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>

#include <sstream>
#include <iostream>
#include <map>
#include <thread>
#include <vector>
#include <mutex>
#include <functional>
#include <any>
#include <condition_variable>
#include "jsonany/Json.h"

using std::string;
using std::function;
using std::vector;
using std::map;
using std::any;

class AmqpServer
{
public:
	int init(string host, int port, string user, string password, string vhost, string exchange, string queue, string serverId, bool sendonly = false);
	int connect();
	int disconnect();
	void reConnect();
    int send(string routingKey, map<string, any> &root);
    static void *ReceiveFunc(void *arg);
    int receiveFunc();
    int addCallback(function<int(string)> &func);
    string getServerId();

private:
	int sendInit();
	int receiveInit();
	
private:
	amqp_socket_t *mSocket;
	amqp_connection_state_t mConnection;
	std::mutex mConnLock;

	std::string mServerId;
	int mChannelNumber;
	std::string mExchangeName;
	std::string mExchangeType = "direct";
	std::string mQueueName;
	std::string mHost;
	std::string mVhost;
	int mPort;
	std::string mUser;
	std::string mUserPwd;
	bool mSendOnly;
    Json mJsoncpp;

    vector<function<int(string)>> mCallbacks;
};
