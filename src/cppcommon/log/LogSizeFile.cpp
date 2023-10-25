#include "log/LogSizeFile.h"
#include "tools/Tools.h"
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
using namespace std;

LogSizeFile::LogSizeFile(std::string uuid, LogLevel level, std::string file, int size, int count)
{
    this->uuid = uuid;
    this->level = level;
    this->mFile = file;
    this->mSize = size;
    this->mCount = count;
    mFp = fopen(file.c_str(), "a");
}

LogSizeFile::~LogSizeFile()
{
    if (mFp) {
        fclose(mFp);
        mFp = nullptr;
    }
}

void LogSizeFile::logFunc(LogLevel level, std::string prefix, std::string& fmt, va_list ap)
{
    if (level <= this->level) {
        fprintf(mFp, "%s", prefix.c_str());
        vfprintf(mFp, fmt.c_str(), ap);
        fflush(mFp);
        long len = ftell(mFp);
        if (mSize > 0 && len >= mSize) {
            fclose(mFp);
            rotate();
            mFp = fopen(mFile.c_str(), "a");
        }
    }
}

void LogSizeFile::logFunc(bool isCrlf, LogLevel level, std::string prefix, std::string_view& fmt, std::format_args args)
{
    if (level <= this->level) {
        string content = std::vformat(fmt, args);
        if (isCrlf) {
            fprintf(mFp, "%s%s\n", prefix.c_str(), content.c_str());
        } else {
            fprintf(mFp, "%s%s", prefix.c_str(), content.c_str());
        }
        fflush(mFp);
        long len = ftell(mFp);
        if (mSize > 0 && len >= mSize) {
            fclose(mFp);
            rotate();
            mFp = fopen(mFile.c_str(), "a");
        }
    }
}

void LogSizeFile::rotate()
{
    if (mCount < 1) {
        return;
    }
    ostringstream oss;
    oss << mFile << "." << mCount;
    string name = oss.str();
    if (exist(name)) {
        remove(name.c_str());
    }
    for (int i = mCount - 1; i > 0; i--) {
        oss.str("");
        oss << mFile << "." << i;
        if (exist(oss.str())) {
            ostringstream oss_new;
            oss_new << mFile << "." << i + 1;
            rename(oss.str().c_str(), oss_new.str().c_str());
        }
    }
    oss.str("");
    oss << mFile << ".1";
    rename(mFile.c_str(), oss.str().c_str());
}

bool LogSizeFile::exist(std::string name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}
