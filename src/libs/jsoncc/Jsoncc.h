#pragma once

#include "log/Log.h"
#include "tools/boost_common.h"
#include "tools/cpp_common.h"
#include "json/config.h"
#include "json/json.h"

class Jsoncc {
public:
    static std::shared_ptr<Json::Value> StringToJson(string jsonstr);
    static string JsonToString(std::shared_ptr<Json::Value>& root);
    static string JsonToString(Json::Value& root);
};
