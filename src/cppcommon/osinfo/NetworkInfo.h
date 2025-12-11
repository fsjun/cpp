#pragma once

#include "log/Log.h"
#include <iostream>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <Winsock2.h>
#include <Windows.h>
#include <Iphlpapi.h>
#include <Ws2tcpip.h>
// #pragma comment(lib, "ws2_32.lib ")
#pragma comment(lib, "Iphlpapi.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#endif

using std::string;
using std::vector;

class NetworkInfo {
public:
    struct CardInfo {
        string name;
        string description;
        string ip;
        string mask;
        string gateway;
        string mac;
    };

    static std::shared_ptr<CardInfo> GetCardInfo();
    static vector<std::shared_ptr<CardInfo>> GetAllCardInfo();

    static string GetInterfaceIp();
    static vector<string> GetAllInterfaceIp();
    static string GetInterfaceMac();
    static bool IsInnerIp(struct in_addr* addr);
    static bool IsInnerIp(string str);
    static bool IsIp(string str);
    static bool IsMac(string str);
};
