#pragma once

class ProcessInfo {
public:
    static int GetPid();
    static int GetParentPid();
    static int GetSid(int pid = 0);
    static int GetGid(int pid = 0);
};


