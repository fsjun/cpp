#pragma once
#include "tools/cpp_common.h"

class Process {
public:
    static int System(string cmd, string& result);
    static int SystemGb18030(string cmd, string& result);
};
