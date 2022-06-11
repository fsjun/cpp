#include "Tools.h"
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#ifndef _WIN32
#include <execinfo.h>
#endif

using namespace std;

std::string Tools::BinToHex(unsigned char* bin, int len)
{
    char* buf = new char[len * 2 + 1]();
    for (int i = 0; i < len; i++) {
        sprintf(buf + 2 * i, "%02x", *(bin + i));
    }
    std::string hex = buf;
    delete[] buf;
    return hex;
}

std::string Tools::LocalTime(std::string fmt)
{
    using std::chrono::system_clock;
    std::time_t tt = system_clock::to_time_t(system_clock::now());

    struct std::tm* ptm = std::localtime(&tt);
    char buf[256];
    std::strftime(buf, sizeof(buf), fmt.c_str(), ptm);
    // std::cout << "Now (local time): " << buf << '\n';
    return buf;
}

std::string Tools::LocalTimeMs(std::string fmt)
{
    using std::chrono::system_clock;
    auto time_now = system_clock::now();
    auto duration_in_ms = chrono::duration_cast<chrono::milliseconds>(time_now.time_since_epoch());
    long long time_ms = duration_in_ms.count();
    long time_s = time_ms / 1000;
    int ms = time_ms % 1000;
    string str = TimestampToLocal(time_s, fmt);
    ostringstream oss;
    oss << str << "." << setfill('0') << setw(3) << ms;
    return oss.str();
}

std::string Tools::TimestampToLocal(long timestamp, std::string fmt)
{
    std::time_t tt = timestamp;
    struct std::tm tm = { 0 };
    struct std::tm* ptm = &tm;
#ifdef _WIN32
    localtime_s(ptm, &tt);
#else
    localtime_r(&tt, ptm);
#endif
    char buf[256];
    std::strftime(buf, sizeof(buf), fmt.c_str(), ptm);
    // std::cout << "Now (local time): " << buf << '\n';
    return buf;
}

long Tools::Now()
{
    using std::chrono::system_clock;
    std::time_t tt = system_clock::to_time_t(system_clock::now());
    return tt;
}

long long Tools::NowMs()
{
    using std::chrono::system_clock;
    auto time_now = system_clock::now();
    auto duration_in_ms = chrono::duration_cast<chrono::milliseconds>(time_now.time_since_epoch());
    long long time_ms = duration_in_ms.count();
    return time_ms;
}

long Tools::StringToTimestamp(string date, string fmt)
{
#ifdef __linux__
    std::tm tm = {};
    strptime(date.c_str(), fmt.c_str(), &tm);
    time_t timestamp = std::mktime(&tm);
    return timestamp;
#else
    return 0;
#endif
}

std::vector<std::string> Tools::Split(std::string ss, char delim)
{
    std::vector<std::string> stu;
    std::stringstream sstr(ss);
    std::string token;
    int be = 0;
    while (std::getline(sstr, token, delim)) {
        stu.push_back(token);
    }
    return stu;
}

std::string Tools::UrlEncode(std::string url)
{
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = url.begin(), n = url.end(); i != n; ++i) {
        string::value_type c = (*i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        // escaped << uppercase;
        escaped << nouppercase;
        escaped << '%' << setw(2) << int((unsigned char)c);
    }
    return escaped.str();
}

std::string Tools::UrlDecode(std::string url)
{
    ostringstream oss;
    char ch;
    int i, ii, len = url.length();

    for (i = 0; i < len; i++) {
        if (url[i] != '%') {
            if (url[i] == '+') {
                oss << ' ';
            } else {
                oss << url[i];
            }
        } else {
            sscanf(url.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            oss << ch;
            i = i + 2;
        }
    }
    return oss.str();
}
