#pragma once

#include "Log.h"

class LogStdOutput : public ILogOutput
{
public:
	LogStdOutput(std::string uuid, LogLevel level);
	virtual void logFunc(LogLevel level,std::string prefix, std::string &fmt, va_list ap);
};