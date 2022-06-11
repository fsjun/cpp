#pragma once

#include "tools/Singleton.h"
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

#define XCT2STR(i) #i
#define CT2STR(l) XCT2STR(l)

#ifndef LOGON
#define LOGON 1
#endif

#ifndef LOGC
#define LOGC 0
#endif

#if LOGON == 0
#define PRINT(module, level, format, ...)
#elif LOGC == 0
#define PRINT(level, format, ...) Log::Print(level, std::string(__FILE__) + std::string(":") + std::string(CT2STR(__LINE__)), format, ##__VA_ARGS__)
#else
#define PRINT(level, format, ...) Log::Print(level, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

#define FATAL(format, ...) PRINT(LOG_LEVEL_FATAL, format, ##__VA_ARGS__)
// #define ERR(format, args...) PRINT(LOG_LEVEL_ERR,format,##args)
#define ERR(format, ...) PRINT(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define WARN(format, ...) PRINT(LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define INFO(format, ...) PRINT(LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define DEBUG(format, ...) PRINT(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

enum LogLevel {
    LOG_LEVEL_NONE = 0x00,
    LOG_LEVEL_FATAL = 0x01,
    LOG_LEVEL_ERROR = 0x02,
    LOG_LEVEL_WARN = 0x04,
    LOG_LEVEL_INFO = 0x08,
    LOG_LEVEL_DEBUG = 0x10,
    LOG_LEVEL_ALL = 0x7fffffff,
};

class ILogOutput {
public:
    virtual ~ILogOutput() { }
    virtual void logFunc(LogLevel level, std::string prefix, std::string& fmt, va_list ap) = 0;
    std::string uuid;
    LogLevel level;
};

class Log : public Singleton<Log> {
public:
    static void Init(LogLevel level, std::string file = "", int size = -1, int count = 1);
    static void AddLogOutput(std::string uuid, std::unique_ptr<ILogOutput>& out);
    static void RemoveLogOutput(std::string uuid);
    static void SetLogLevel(LogLevel level);
    static void Print(LogLevel level, char* file, int line, char* format, ...);
    static void Print(LogLevel level, std::string fileLine, std::string format, ...);
    static LogLevel StringToLevel(std::string level);
    static long GetTid();

private:
    static std::string LevelToString(LogLevel level);
    void addLogOutput(std::string uuid, std::unique_ptr<ILogOutput>& out);
    void removeLogOutput(std::string uuid);

private:
    static std::mutex sMtx;
    std::map<std::string, std::unique_ptr<ILogOutput>> mOut;
    std::unique_ptr<ILogOutput> mStdOut;

    static std::map<std::string, LogLevel> sStringLevelMap;
    static std::map<LogLevel, std::string> sLevelStringMap;
};
