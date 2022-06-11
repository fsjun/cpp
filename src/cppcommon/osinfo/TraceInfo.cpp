#include "TraceInfo.h"
#include <sstream>

#ifdef __linux__
#include <execinfo.h>

std::unique_ptr<TraceInfo> TraceInfo::GetAllTraceInfo()
{
    int size = 0;
    int max = 10;
    std::unique_ptr<TraceInfo> traceInfo(new TraceInfo());
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
    traceInfo->backTrace.assign(buffer, buffer + size);
    delete[] buffer;
    return traceInfo;
}

std::unique_ptr<TraceInfo> TraceInfo::GetTraceInfo()
{
    int size = 10;
    std::unique_ptr<TraceInfo> traceInfo(new TraceInfo());
    traceInfo->backTrace.assign(size, nullptr);
    void** buffer = traceInfo->backTrace.data();
    size = backtrace(buffer, size);
    return traceInfo;
}

std::string TraceInfo::getBackTraceSymbols()
{
    std::ostringstream oss;
    char** strings = nullptr;
    void** buffer = backTrace.data();
    int size = backTrace.size();
    strings = backtrace_symbols(buffer, size);
    for (int i = 0; i < size; i++) {
        oss << strings[i] << std::endl;
    }
    free(strings);
    return oss.str();
}
#else

std::unique_ptr<TraceInfo> TraceInfo::GetAllTraceInfo()
{
    std::unique_ptr<TraceInfo> traceInfo(new TraceInfo());
    return traceInfo;
}

std::unique_ptr<TraceInfo> TraceInfo::GetTraceInfo()
{
    std::unique_ptr<TraceInfo> traceInfo(new TraceInfo());
    return traceInfo;
}

std::string TraceInfo::getBackTraceSymbols()
{
    std::ostringstream oss;
    return oss.str();
}

#endif
