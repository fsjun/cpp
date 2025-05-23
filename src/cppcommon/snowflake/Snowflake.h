#pragma once

#include "tools/Singleton.h"
#include "snowflake/SnowflakeT.h"

class Snowflake : public Singleton<Snowflake> {
public:
    Snowflake();
    int64_t nextid();

private:
    snowflake<1735660800000, std::mutex> mGenerator;
};
