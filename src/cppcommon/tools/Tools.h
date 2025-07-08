#pragma once

#include <iostream>
#include <vector>

class Tools {
public:
    static std::string BinToHex(unsigned char* bin, int len);
    static std::string LocalTime(std::string fmt = "%F %T");
    static std::string LocalTimeMs(std::string fmt = "%F %T");
    static std::string TimestampToLocalMs(long long timestamp, std::string fmt = "%F %T");
    // %Y-%m-%d-%H-%M-%S
    static std::string TimestampToLocal(long timestamp, std::string fmt = "%F %T");
    static long StringToTimestamp(std::string date, std::string fmt = "%F %T");
    static long Now();
    static long long NowMs();
    static std::vector<std::string> Split(std::string ss, char delim);
    static std::string UrlEncode(std::string url);
    static std::string UrlDecode(std::string url);
};
