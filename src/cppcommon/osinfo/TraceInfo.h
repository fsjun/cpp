#pragma once

#include <iostream>
#include <memory>
#include <vector>

class TraceInfo {
public:
    static std::unique_ptr<TraceInfo> GetAllTraceInfo();
    static std::unique_ptr<TraceInfo> GetTraceInfo();
    std::string getBackTraceSymbols();

private:
    std::vector<void*> backTrace;
};
