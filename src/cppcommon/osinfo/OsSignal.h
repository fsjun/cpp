#pragma once

#include "tools/cpp_common.h"
#ifndef _WIN32
#include <execinfo.h>
#endif
#include <setjmp.h>
#include <signal.h>

class OsSignal {
public:
    static void Install();
    static int Recover();
    static std::string GetBackTrace();

private:
    static void handle_signal(int signum);
    static thread_local jmp_buf env;
};
