#pragma once

#include "Log.h"

class LogStdOutput : public ILogOutput
{
public:
    LogStdOutput(std::string uuid, LogLevel level);
    virtual void logFunc(LogLevel level,std::string prefix, std::string &fmt, va_list ap);
    virtual void logFunc(bool isCrlf, LogLevel level, std::string prefix, std::string_view& fmt, std::format_args args);
};