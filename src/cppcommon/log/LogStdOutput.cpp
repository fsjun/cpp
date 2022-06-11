#include "log/LogStdOutput.h"
#include <iostream>
#include <string>
#include <sstream>
#include "tools/Tools.h"

LogStdOutput::LogStdOutput(std::string uuid, LogLevel level)
{
	this->uuid = uuid;
	this->level = level;
}

void LogStdOutput::logFunc(LogLevel level,std::string prefix, std::string &fmt, va_list ap)
{
    if (level <= this->level) {
        printf("%s", prefix.c_str());
        vprintf(fmt.c_str(), ap);
        fflush(stdout);
    }
}
