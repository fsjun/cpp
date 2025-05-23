#include "OsSignal.h"

thread_local jmp_buf OsSignal::env;

void OsSignal::Install()
{
    // signal(SIGSEGV, handle_signal);
}

int OsSignal::Recover()
{
    int ret = setjmp(env);
    if (ret) {
        INFOLN("recover from signal, signum:{}\n", ret);
    }
    return ret;
}

void OsSignal::handle_signal(int signum)
{
    string bt = GetBackTrace();
    ERRLN("receive signal, signum:{} backtrace:{}\n", signum, bt);
    longjmp(env, signum);
}

std::string OsSignal::GetBackTrace()
{
    ostringstream oss;
#ifndef _WIN32
    int size = 0;
    int max = 10;
    void** buffer = nullptr;
    while (true) {
        buffer = new void*[max]();
        size = backtrace(buffer, max);
        if (size != max) {
            break;
        }
        max += size;
        delete[] buffer;
    }
    char** strings = nullptr;
    strings = backtrace_symbols(buffer, size);
    delete[] buffer;
    for (int i = 0; i < size; i++) {
        oss << strings[i] << std::endl;
    }
    free(strings);
#endif
    return oss.str();
}
