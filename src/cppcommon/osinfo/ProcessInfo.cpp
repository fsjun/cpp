#include "ProcessInfo.h"
#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#include <WtsApi32.h>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Wtsapi32.lib")

#define MAX_PROCESS_LENGTH (128)

int ProcessInfo::GetPid()
{
    DWORD processId = GetCurrentProcessId();
    return processId;
}

int ProcessInfo::GetParentPid()
{
    ULONG_PTR pbi[6];
    ULONG ulSize = 0;
    LONG(WINAPI * NtQueryInformationProcess)
    (HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
    *(FARPROC*)&NtQueryInformationProcess = GetProcAddress(LoadLibraryA("NTDLL.DLL"), "NtQueryInformationProcess");

    DWORD processId = GetCurrentProcessId();
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

    if (NtQueryInformationProcess) {
        if (NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
            return pbi[5];
    }
    return -1;
}

int ProcessInfo::GetSid(int pid)
{
    return 0;
}

int ProcessInfo::GetGid(int pid)
{
    return 0;
}
#else
#include <unistd.h>

int ProcessInfo::GetPid()
{
    pid_t pid = getpid();
    return pid;
}

int ProcessInfo::GetParentPid()
{
    pid_t pid = getppid();
    return pid;
}

int ProcessInfo::GetSid(int pid)
{
    return getsid(pid);
}

int ProcessInfo::GetGid(int pid)
{
    return getpgid(pid);
}
#endif
