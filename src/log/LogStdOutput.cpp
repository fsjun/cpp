#include "log/LogStdOutput.h"
#include <iostream>
#include <string>

LogStdOutput::LogStdOutput(std::string uuid, LogLevel level)
{
    this->uuid = uuid;
    this->level = level;
}

void LogStdOutput::logFunc(LogLevel level, std::string prefix, std::string& fmt, va_list ap)
{
    if (level <= this->level) {
        printf("%s", prefix.c_str());
        vprintf(fmt.c_str(), ap);
        fflush(stdout);
    }
}

void LogStdOutput::logFunc(bool isCrlf, LogLevel level, std::string prefix, std::string_view& fmt, std::format_args args)
{
    if (level <= this->level) {
        if (isCrlf) {
            std::cout << prefix << std::vformat(fmt, args) << std::endl;
        } else {
            std::cout << prefix << std::vformat(fmt, args) << std::flush;
        }
    }
}
