#include "log/Log.h"
#include "LogSizeFile.h"
#include "LogStdOutput.h"
#include "tools/Tools.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include <utility>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/syscall.h>
#include <unistd.h>
#endif

std::mutex Log::sMtx;

std::map<std::string, LogLevel> Log::sStringLevelMap = {
    { "none", LOG_LEVEL_NONE },
    { "fatal", LOG_LEVEL_FATAL },
    { "error", LOG_LEVEL_ERROR },
    { "warn", LOG_LEVEL_WARN },
    { "info", LOG_LEVEL_INFO },
    { "debug", LOG_LEVEL_DEBUG },
    { "all", LOG_LEVEL_ALL }
};

std::map<LogLevel, std::string> Log::sLevelStringMap = {
    { LOG_LEVEL_NONE, "none" },
    { LOG_LEVEL_FATAL, "fatal" },
    { LOG_LEVEL_ERROR, "error" },
    { LOG_LEVEL_WARN, "warn" },
    { LOG_LEVEL_INFO, "info" },
    { LOG_LEVEL_DEBUG, "debug" },
    { LOG_LEVEL_ALL, "all" }
};

void Log::Init(LogLevel level, std::string file, int size, int count)
{
    std::lock_guard<std::mutex> l(sMtx);
    auto log = GetInstance();
    log->mStdOut.reset(new LogStdOutput("stdout", level));
    if (!file.empty()) {
        std::unique_ptr<ILogOutput> fileLog(new LogSizeFile("file", level, std::move(file), size, count));
        log->mOut.emplace("file", std::move(fileLog));
    }
}

void Log::AddLogOutput(std::string uuid, std::unique_ptr<ILogOutput>& out)
{
    std::lock_guard<std::mutex> l(sMtx);
    if (!sInstance) {
        return;
    }
    sInstance->addLogOutput(uuid, out);
}

void Log::RemoveLogOutput(std::string uuid)
{
    std::lock_guard<std::mutex> l(sMtx);
    if (!sInstance) {
        return;
    }
    sInstance->removeLogOutput(uuid);
}

void Log::SetLogLevel(LogLevel level)
{
    std::lock_guard<std::mutex> l(sMtx);
    if (!sInstance) {
        return;
    }
    sInstance->mStdOut->level = level;
    for (auto& it : sInstance->mOut) {
        auto& out = it.second;
        out->level = level;
    }
}

long Log::GetTid()
{
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

void Log::Print(LogLevel level, char* file, int line, char* format, ...)
{
    va_list ap;
    std::ostringstream oss;
    oss << file << ":" << line;
    std::string fileLine = oss.str();
    std::string fmt = format;
    int pos = fileLine.find_last_of("/");
    if (pos != std::string::npos) {
        fileLine = fileLine.substr(pos + 1);
    }
    oss.str("");
    std::lock_guard<std::mutex> l(sMtx);
    if (!sInstance) {
        return;
    }
    std::string currentTime = Tools::LocalTimeMs();
    oss << "[" << currentTime << " " << GetTid() << " " << fileLine << "] <" << LevelToString(level) << "> ";
    va_start(ap, format);
    sInstance->mStdOut->logFunc(level, oss.str(), fmt, ap);
    va_end(ap);
    for (auto& it : sInstance->mOut) {
        auto& out = it.second;
        va_start(ap, format);
        out->logFunc(level, oss.str(), fmt, ap);
        va_end(ap);
    }
}

void Log::Print(LogLevel level, std::string fileLine, std::string fmt, ...)
{
    va_list ap;
#if _WIN32
    int pos = fileLine.find_last_of("\\");
    if (pos != std::string::npos) {
        fileLine = fileLine.substr(pos + 1);
    }
#else
    int pos = fileLine.find_last_of("/");
    if (pos != std::string::npos) {
        fileLine = fileLine.substr(pos + 1);
    }
#endif
    std::stringstream oss;
    std::lock_guard<std::mutex> l(sMtx);
    if (!sInstance) {
        return;
    }
    std::string currentTime = Tools::LocalTimeMs();
    oss << "[" << currentTime << " " << GetTid() << " " << fileLine << "] <" << LevelToString(level) << "> ";
    va_start(ap, fmt);
    sInstance->mStdOut->logFunc(level, oss.str(), fmt, ap);
    va_end(ap);
    for (auto& it : sInstance->mOut) {
        auto& out = it.second;
        va_start(ap, fmt);
        out->logFunc(level, oss.str(), fmt, ap);
        va_end(ap);
    }
}

void Log::addLogOutput(std::string uuid, std::unique_ptr<ILogOutput>& out)
{
    mOut.insert(std::pair<std::string, std::unique_ptr<ILogOutput>>(uuid, std::move(out)));
}

void Log::removeLogOutput(std::string uuid)
{
    mOut.erase(uuid);
}

std::string Log::LevelToString(LogLevel level)
{
    auto it = sLevelStringMap.find(level);
    if (it == sLevelStringMap.end()) {
        return "info";
    }
    return it->second;
}

LogLevel Log::StringToLevel(std::string level)
{
    std::lock_guard<std::mutex> l(sMtx);
    auto it = sStringLevelMap.find(level);
    if (it == sStringLevelMap.end()) {
        return LOG_LEVEL_INFO;
    }
    return it->second;
}
