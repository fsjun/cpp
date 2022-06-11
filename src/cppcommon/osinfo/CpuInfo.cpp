#include "CpuInfo.h"
#if WIN32
#include "windows.h"
#else
#include "unistd.h"
#endif

int CpuInfo::GetCpuCount() {
    int num = 0;
#if WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    num = sysInfo.dwNumberOfProcessors;
#else
    num = sysconf(_SC_NPROCESSORS_CONF);
#endif
    return num;
}

int CpuInfo::GetUserCpuCount() {
    int num = 0;
#if WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    num = sysInfo.dwNumberOfProcessors;
#else
    // enabled cpu count
    num = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    return num;
}
