#pragma once

#include "Log.h"
#include <fstream>
#include "stdio.h"

class LogSizeFile : public ILogOutput
{
public:
    LogSizeFile(std::string uuid, LogLevel level, std::string file, int size, int count);
    ~LogSizeFile();
    virtual void logFunc(LogLevel level, std::string prefix, std::string& fmt, va_list ap);
    virtual void logFunc(bool isCrlf, LogLevel level, std::string prefix, std::string_view& fmt, std::format_args args);

private:
	void rotate();
	bool exist(std::string name);

private:
	std::string mFile;
	int mSize;
	int mCount;
	FILE *mFp;
};
