#pragma once
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <string>
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

#define EOL "\n"

#define FATAL(format, ...) SPDLOG_LOGGER_CRITICAL(spdlog::get("log"), format, ##__VA_ARGS__)
#define ERR(format, ...) SPDLOG_LOGGER_ERROR(spdlog::get("log"), format, ##__VA_ARGS__)
#define WARN(format, ...) SPDLOG_LOGGER_WARN(spdlog::get("log"), format, ##__VA_ARGS__)
#define INFO(format, ...) SPDLOG_LOGGER_INFO(spdlog::get("log"), format, ##__VA_ARGS__)
#define DEBUG(format, ...) SPDLOG_LOGGER_DEBUG(spdlog::get("log"), format, ##__VA_ARGS__)

#define FATALLN(format, ...) SPDLOG_LOGGER_CRITICAL(spdlog::get("log"), format##EOL, ##__VA_ARGS__)
#define ERRLN(format, ...) SPDLOG_LOGGER_ERROR(spdlog::get("log"), format##EOL, ##__VA_ARGS__)
#define WARNLN(format, ...) SPDLOG_LOGGER_WARN(spdlog::get("log"), format##EOL, ##__VA_ARGS__)
#define INFOLN(format, ...) SPDLOG_LOGGER_INFO(spdlog::get("log"), format##EOL, ##__VA_ARGS__)
#define DEBUGLN(format, ...) SPDLOG_LOGGER_DEBUG(spdlog::get("log"), format##EOL, ##__VA_ARGS__)

// INITLOG(level, file, size, count, console)
#define INITLOG(...) init_log(__VA_ARGS__)

enum LogLevel {
    LOG_LEVEL_NONE = 0x00,
    LOG_LEVEL_FATAL = SPDLOG_LEVEL_CRITICAL,
    LOG_LEVEL_ERROR = SPDLOG_LEVEL_ERROR,
    LOG_LEVEL_WARN = SPDLOG_LEVEL_WARN,
    LOG_LEVEL_INFO = SPDLOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG = SPDLOG_LEVEL_DEBUG,
    LOG_LEVEL_ALL = 0x7fffffff,
};

void init_log(LogLevel level = LOG_LEVEL_INFO, std::string file = "", int size = 1024 * 1024 * 10, int count = 3, bool console = true);
