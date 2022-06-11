#include "NetworkInfo.h"

string NetworkInfo::GetInterfaceIp()
{
    vector<string> vec = GetAllInterfaceIp();
    if (vec.empty()) {
        return "";
    }
    return vec[0];
}

#ifdef _WIN32
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
                if (ip_str != "0.0.0.0" && strncmp(ip_str.c_str(), "169.254", 7) != 0 && ip_str != "127.0.0.1") {
                    vec.emplace_back(ip_str);
                }
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
            if (ip != 0x7f000001) {
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, addr, addressBuffer, INET_ADDRSTRLEN);
                vec.emplace_back(addressBuffer);
            }
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
