#include "NetworkInfo.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <iptypes.h>
#else
#include <arpa/inet.h>
#endif
#include <iomanip>
#include <regex>
#include <sstream>
#include <string.h>

std::shared_ptr<NetworkInfo::CardInfo> NetworkInfo::GetCardInfo()
{
    auto vec = GetAllCardInfo();
    if (vec.empty()) {
        return nullptr;
    }
    std::shared_ptr<NetworkInfo::CardInfo> cardInfo;
    for (auto val : vec) {
        string name = val->name;
        if (name.starts_with("e")) {
            cardInfo = val;
            break;
        }
        string ip_str = val->ip;
        if (!ip_str.empty() && ip_str != "0.0.0.0" && strncmp(ip_str.c_str(), "169.254", 7) != 0 && ip_str != "127.0.0.1") {
            cardInfo = val;
        }
    }
    return cardInfo;
}

string NetworkInfo::GetInterfaceIp()
{
    auto cardInfo = GetCardInfo();
    return cardInfo ? cardInfo->ip : "";
}

string NetworkInfo::GetInterfaceMac()
{
    auto cardInfo = GetCardInfo();
    return cardInfo ? cardInfo->mac : "";
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
#else

static string getMac(string name)
{
    struct ifreq ifreq;
    int sock = 0;
    char mac[32] = "";
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        int err = errno;
        char* msg = strerror(err);
        ERRLN("socket error {}:{}", err, msg ? msg : "");
        return "";
    }
    strcpy(ifreq.ifr_name, name.c_str());
    if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0) {
        int err = errno;
        char* msg = strerror(err);
        ERRLN("ioctl error {}:{}", err, msg ? msg : "");
        return "";
    }
    snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
        (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
        (unsigned char)ifreq.ifr_hwaddr.sa_data[1],
        (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
        (unsigned char)ifreq.ifr_hwaddr.sa_data[3],
        (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
        (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
    return mac;
}

vector<std::shared_ptr<NetworkInfo::CardInfo>> NetworkInfo::GetAllCardInfo()
{
    vector<std::shared_ptr<NetworkInfo::CardInfo>> vec;
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;
    struct in_addr* addr = nullptr;

    int ret = getifaddrs(&ifAddrStruct);
    if (ret < 0) {
        return vec;
    }
    ifa = ifAddrStruct;
    while (ifa != nullptr) {
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            char addressBuffer[INET_ADDRSTRLEN];
            auto cardInfo = std::make_shared<CardInfo>();
            // name
            cardInfo->name = ifa->ifa_name ? ifa->ifa_name : "";
            // ip
            addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            memset(addressBuffer, 0, sizeof(addressBuffer));
            inet_ntop(AF_INET, addr, addressBuffer, INET_ADDRSTRLEN);
            cardInfo->ip = addressBuffer;
            // subnet mask
            addr = &((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr;
            memset(addressBuffer, 0, sizeof(addressBuffer));
            inet_ntop(AF_INET, addr, addressBuffer, INET_ADDRSTRLEN);
            cardInfo->mask = addressBuffer;
            // mac
            cardInfo->mac = getMac(cardInfo->name);
            INFOLN("{} IPv4 Address {} mask {} mac {}", cardInfo->name, cardInfo->ip, cardInfo->mask, cardInfo->mac);
            vec.emplace_back(cardInfo);
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, addr, addressBuffer, INET6_ADDRSTRLEN);
            INFOLN("{} IPv6 Address {}", ifa->ifa_name, addressBuffer);
        }
        ifa = ifa->ifa_next;
    }
    if (ifAddrStruct) {
        freeifaddrs(ifAddrStruct);
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
