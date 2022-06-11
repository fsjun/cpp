#pragma once

#include "log/Log.h"
#include <iostream>
#include <vector>
#ifdef _WIN32
#include <Windows.h>

#include <Iphlpapi.h>
#include <Winsock2.h>
//#pragma comment(lib, "ws2_32.lib ")
#pragma comment(lib, "Iphlpapi.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/types.h>
#endif

using std::string;
using std::vector;

class NetworkInfo {
public:
    static string GetInterfaceIp();
    static vector<string> GetAllInterfaceIp();
    static bool IsInnerIp(struct in_addr* addr);
};
