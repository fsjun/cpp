#include "snowflake/Snowflake.h"
#include <random>
#include "log/Log.h"

Snowflake::Snowflake()
{
    std::default_random_engine e;
    std::uniform_int_distribution<int> u(0,31);
    e.seed(time(0));
    int64_t workerId = u(e);
    int64_t datacenterId = u(e);
    mGenerator.init(workerId, datacenterId);
    INFOLN("snowflake workerId:{} datacenterId:{}", workerId, datacenterId);
}

int64_t Snowflake::nextid()
{
    return mGenerator.nextid();
}
