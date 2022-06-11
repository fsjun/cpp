#include "TimeZone.h"
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32

void TimeZone::SetTimeZone(string zone)
{
}

#else

void TimeZone::SetTimeZone(string zone)
{
    setenv("TZ", "Asia/Shanghai", 1);
    tzset();
}

#endif
