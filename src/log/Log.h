#pragma once

#include "tools/Singleton.h"
#include <condition_variable>
#include <format>
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
#define PRINT(crlf, level, format, ...)
#elif LOGC == 0
#define PRINT(crlf, level, format, ...) Log::PrintT(crlf, level, std::string(__FILE__) + std::string(":") + std::string(CT2STR(__LINE__)), format, ##__VA_ARGS__)
#else
#define PRINT(crlf, level, format, ...) Log::Print(level, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

// #define FATAL(format, args...) PRINT(LOG_LEVEL_ERR,format,##args)
#define FATAL(format, ...) PRINT(false, LOG_LEVEL_FATAL, format, ##__VA_ARGS__)
#define ERR(format, ...) PRINT(false, LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define WARN(format, ...) PRINT(false, LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define INFO(format, ...) PRINT(false, LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define DEBUG(format, ...) PRINT(false, LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

#define FATALLN(format, ...) PRINT(true, LOG_LEVEL_FATAL, format, ##__VA_ARGS__)
#define ERRLN(format, ...) PRINT(true, LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define WARNLN(format, ...) PRINT(true, LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define INFOLN(format, ...) PRINT(true, LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define DEBUGLN(format, ...) PRINT(true, LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

// INITLOG(level, file, size, count)
#define INITLOG(...) Log::Init(__VA_ARGS__)
// void init_log(LogLevel level = LOG_LEVEL_INFO, std::string file = "", int size = 1024 * 1024 * 10, int count = 3, bool console = true);

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
    virtual void logFunc(bool isCrlf, LogLevel level, std::string prefix, std::string_view& fmt, std::format_args args) = 0;
    std::string uuid;
    LogLevel level;
};

class Log : public Singleton<Log> {
public:
    static void Init(LogLevel level = LOG_LEVEL_INFO, std::string file = "", int size = 1024*1024*10, int count = 3, bool console = true);
    static void AddLogOutput(std::string uuid, std::unique_ptr<ILogOutput>& out);
    static void RemoveLogOutput(std::string uuid);
    static void SetLogLevel(LogLevel level);
    static void Print(LogLevel level, char* file, int line, char* format, ...);
    static void Print(LogLevel level, std::string fileLine, std::string format, ...);
    template <typename... Args>
    static void PrintT(bool isCrlf, LogLevel level, std::string fileLine, std::string_view format, Args&&... args)
    {
        Print(isCrlf, level, fileLine, format, std::make_format_args(args...));
    }
    static void Print(bool isCrlf, LogLevel level, std::string fileLine, std::string_view format, std::format_args args);
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
