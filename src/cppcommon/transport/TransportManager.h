#pragma once

#include "log/Log.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"
#include <algorithm>
#include <boost/asio.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <Iphlpapi.h>
#include <Winsock2.h>
//#pragma comment(lib, "ws2_32.lib ")
#pragma comment(lib, "Iphlpapi.lib")
#else
#include <ifaddrs.h>
#include <sys/types.h>
#endif

class TcpTransport;
class UdpTransport;

enum TransportTcpType {
    TRANSPORT_TCP_TYPE_CLIENT,
    TRANSPORT_TCP_TYPE_SERVER,
    TRANSPORT_TCP_TYPE_ACCEPT
};

enum TransportTcpEventType {
    TRANSPORT_TCP_EVENT_TYPE_ON_CONNECT,
    TRANSPORT_TCP_EVENT_TYPE_ON_ACCEPT
};

class TransportManager {
public:
    TransportManager(shared_ptr<boost::asio::io_context> ioc = nullptr);

    shared_ptr<TcpTransport> addTcpTransport();
    shared_ptr<UdpTransport> addUdpTransport();
    string getLocalIp();
    bool isInnerIp(struct in_addr* addr);
    void run();
    void stop();
    shared_ptr<boost::asio::io_context> getIoContext();

private:
    shared_ptr<boost::asio::io_context> mIoContext;
};
