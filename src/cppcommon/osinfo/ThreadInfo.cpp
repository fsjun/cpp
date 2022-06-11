#include "ThreadInfo.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#endif

long ThreadInfo::GetTid() {
#if __linux__
    #define gettidv1() syscall(__NR_gettid)
    #define gettidv2() syscall(SYS_gettid)
    return gettidv1();
#elif _WIN32
    return GetCurrentThreadId();
#else
    uint64_t tid;
    pthread_threadid_np(nullptr, &tid);
    return tid;
#endif
}
