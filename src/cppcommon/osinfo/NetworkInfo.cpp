#include "NetworkInfo.h"
#include <iomanip>
#include <regex>
#include <sstream>

std::shared_ptr<NetworkInfo::CardInfo> NetworkInfo::GetCardInfo()
{
    auto vec = GetAllCardInfo();
    if (vec.empty()) {
        return nullptr;
    }
    for (auto cardInfo : vec) {
        string ip_str = cardInfo->ip;
        if (!ip_str.empty() && ip_str != "0.0.0.0" && strncmp(ip_str.c_str(), "169.254", 7) != 0 && ip_str != "127.0.0.1") {
            return cardInfo;
        }
    }
    return nullptr;
}

string NetworkInfo::GetInterfaceIp()
{
    vector<string> vec = GetAllInterfaceIp();
    if (vec.empty()) {
        return "";
    }
    for (string ip_str : vec) {
        if (!ip_str.empty() && ip_str != "0.0.0.0" && strncmp(ip_str.c_str(), "169.254", 7) != 0 && ip_str != "127.0.0.1") {
            return ip_str;
        }
    }
    return "";
}

string NetworkInfo::GetInterfaceMac()
{
    auto vec = GetAllCardInfo();
    if (vec.empty()) {
        return "";
    }
    for (auto cardInfo : vec) {
        string ip_str = cardInfo->ip;
        if (!ip_str.empty() && ip_str != "0.0.0.0" && strncmp(ip_str.c_str(), "169.254", 7) != 0 && ip_str != "127.0.0.1") {
            return cardInfo->mac;
        }
    }
    return "";
}

#ifdef _WIN32
vector<std::shared_ptr<NetworkInfo::CardInfo>> NetworkInfo::GetAllCardInfo()
{
    vector<std::shared_ptr<NetworkInfo::CardInfo>> vec;
    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
    unsigned long stSize = sizeof(IP_ADAPTER_INFO);
    string ip;
    int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    DWORD netCardNum = 0;
    GetNumberOfInterfaces(&netCardNum);
    netCardNum = 0;
    int IPnumPerNetCard = 0;
    if (ERROR_BUFFER_OVERFLOW == nRel) {
        delete pIpAdapterInfo;
        pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[stSize];
        nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    }
    if (ERROR_SUCCESS == nRel) {
        PIP_ADAPTER_INFO orig = pIpAdapterInfo;
        while (pIpAdapterInfo) {
            switch (pIpAdapterInfo->Type) {
            case MIB_IF_TYPE_OTHER:
                // cout << "OTHER" << endl;
                break;
            case MIB_IF_TYPE_ETHERNET:
                // cout << "ETHERNET" << endl;
                break;
            case MIB_IF_TYPE_TOKENRING:
                // cout << "TOKENRING" << endl;
                break;
            case MIB_IF_TYPE_FDDI:
                // cout << "FDDI" << endl;
                break;
            case MIB_IF_TYPE_PPP:
                // cout << "PPP" << endl;
                break;
            case MIB_IF_TYPE_LOOPBACK:
                // cout << "LOOPBACK" << endl;
                break;
            case MIB_IF_TYPE_SLIP:
                // cout << "SLIP" << endl;
                break;
            default:
                // cout << "" << endl;
                break;
            }
            std::ostringstream oss;
            for (DWORD i = 0; i < pIpAdapterInfo->AddressLength; i++) {
                if (i < pIpAdapterInfo->AddressLength - 1) {
                    oss << std::setfill('0') << std::setw(2) << std::hex << std::setiosflags(std::ios::uppercase) << int(pIpAdapterInfo->Address[i]) << "-";
                } else {
                    oss << std::setfill('0') << std::setw(2) << std::hex << std::setiosflags(std::ios::uppercase) << int(pIpAdapterInfo->Address[i]);
                }
            }
            string mac = oss.str();
            string gateway = pIpAdapterInfo->GatewayList.IpAddress.String;
            IP_ADDR_STRING* pIpAddrString = &(pIpAdapterInfo->IpAddressList);
            do {
                auto cardInfo = std::make_shared<CardInfo>();
                cardInfo->name = pIpAdapterInfo->AdapterName;
                cardInfo->description = pIpAdapterInfo->Description;
                cardInfo->ip = pIpAddrString->IpAddress.String;
                cardInfo->mask = pIpAddrString->IpMask.String;
                cardInfo->gateway = gateway;
                cardInfo->mac = mac;
                vec.emplace_back(cardInfo);
                pIpAddrString = pIpAddrString->Next;
            } while (pIpAddrString);
            pIpAdapterInfo = pIpAdapterInfo->Next;
            // cout << "--------------------------------------------------------------------" << endl;
        }
        pIpAdapterInfo = orig;
    }
    if (pIpAdapterInfo) {
        delete pIpAdapterInfo;
    }
    return vec;
}

vector<string> NetworkInfo::GetAllInterfaceIp()
{
    vector<string> vec;
    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
    unsigned long stSize = sizeof(IP_ADAPTER_INFO);
    string ip;
    int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    DWORD netCardNum = 0;
    GetNumberOfInterfaces(&netCardNum);
    netCardNum = 0;
    int IPnumPerNetCard = 0;
    if (ERROR_BUFFER_OVERFLOW == nRel) {
        delete pIpAdapterInfo;
        pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[stSize];
        nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    }
    if (ERROR_SUCCESS == nRel) {
        while (pIpAdapterInfo) {
            switch (pIpAdapterInfo->Type) {
            case MIB_IF_TYPE_OTHER:
                // cout << "OTHER" << endl;
                break;
            case MIB_IF_TYPE_ETHERNET:
                // cout << "ETHERNET" << endl;
                break;
            case MIB_IF_TYPE_TOKENRING:
                // cout << "TOKENRING" << endl;
                break;
            case MIB_IF_TYPE_FDDI:
                // cout << "FDDI" << endl;
                break;
            case MIB_IF_TYPE_PPP:
                // cout << "PPP" << endl;
                break;
            case MIB_IF_TYPE_LOOPBACK:
                // cout << "LOOPBACK" << endl;
                break;
            case MIB_IF_TYPE_SLIP:
                // cout << "SLIP" << endl;
                break;
            default:
                // cout << "" << endl;
                break;
            }
            for (DWORD i = 0; i < pIpAdapterInfo->AddressLength; i++) {
                if (i < pIpAdapterInfo->AddressLength - 1) {
                    // printf("%02X-", pIpAdapterInfo->Address[i]);
                } else {
                    // printf("%02X\n", pIpAdapterInfo->Address[i]);
                }
            }
            IPnumPerNetCard = 0;
            IP_ADDR_STRING* pIpAddrString = &(pIpAdapterInfo->IpAddressList);
            do {
                // cout << "ip count" << ++IPnumPerNetCard << endl;
                // cout << "IP addr：" << pIpAddrString->IpAddress.String << endl;
                // cout << "netmask：" << pIpAddrString->IpMask.String << endl;
                // cout << "gateway：" << pIpAdapterInfo->GatewayList.IpAddress.String << endl;
                string ip_str = pIpAddrString->IpAddress.String;
                vec.emplace_back(ip_str);
                pIpAddrString = pIpAddrString->Next;
            } while (pIpAddrString);
            pIpAdapterInfo = pIpAdapterInfo->Next;
            // cout << "--------------------------------------------------------------------" << endl;
        }
    }
    if (pIpAdapterInfo) {
        delete pIpAdapterInfo;
    }
    return vec;
}
#else
vector<std::shared_ptr<NetworkInfo::CardInfo>> NetworkInfo::GetAllCardInfo()
{
    vector<std::shared_ptr<NetworkInfo::CardInfo>> vec;
    return vec;
}

vector<string> NetworkInfo::GetAllInterfaceIp()
{
    vector<string> vec;
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;
    struct in_addr* addr = nullptr;

    int ret = getifaddrs(&ifAddrStruct);
    if (ret < 0) {
        return vec;
    }
    ifa = ifAddrStruct;
    while (ifAddrStruct != nullptr) {
        if (ifAddrStruct->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            addr = &((struct sockaddr_in*)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET, addr, addressBuffer, INET6_ADDRSTRLEN);
            INFO("%s IPv4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
            long ip = (long)ntohl(addr->s_addr);
            // if (ip != 0x7f000001) {
            char addressBuffer2[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, addr, addressBuffer2, INET_ADDRSTRLEN);
            vec.emplace_back(addressBuffer2);
            //}
        } else if (ifAddrStruct->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            addr = &((struct sockaddr_in*)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, addr, addressBuffer, INET6_ADDRSTRLEN);
            INFO("%s IPv6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    if (ifa) {
        freeifaddrs(ifa);
    }
    return vec;
}
#endif

bool NetworkInfo::IsInnerIp(struct in_addr* addr)
{
    long ip = (long)ntohl(addr->s_addr);
    if ((ip >= 0x0A000000 && ip <= 0x0AFFFFFF) || // 10.0.0.0 ~ 10.255.255.255
        (ip >= 0xAC100000 && ip <= 0xAC1FFFFF) || // 172.16.0.0 ~ 172.31.255.255
        (ip >= 0xC0A80000 && ip <= 0xC0A8FFFF) // 192.168.0.0 ~ 192.168.255.255
    ) {
        return true;
    }
    return false;
}

bool NetworkInfo::IsInnerIp(string str)
{
    struct in_addr addr;
    int ret = inet_pton(AF_INET, str.c_str(), &addr);
    if (ret <= 0) {
        return false;
    }
    return IsInnerIp(&addr);
}

bool NetworkInfo::IsIp(string str)
{
    std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
    std::smatch res;
    if (regex_match(str, res, pattern)) {
        return true;
    }
    return false;
}

bool NetworkInfo::IsMac(string str)
{
    std::regex pattern("(^[A-Fa-f\\d]{2}\\.[A-Fa-f\\d]{2}\\.[A-Fa-f\\d]{2}\\.[A-Fa-f\\d]{2}\\.[A-Fa-f\\d]{2}\\.[A-Fa-f\\d]{2}$)");

    //    let reg = /^[A-Fa-f\d]{2}.[A-Fa-f\d]{2}.[A-Fa-f\d]{2}.[A-Fa-f\d]{2}.[A-Fa-f\d]{2}.[A-Fa-f\d]{2}$/;
    std::smatch res;
    if (regex_match(str, res, pattern)) {
        return true;
    }
    return false;
}
