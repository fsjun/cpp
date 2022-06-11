
#include "Jsoncc.h"

std::shared_ptr<Json::Value> Jsoncc::StringToJson(string jsonstr)
{
    auto root = std::make_shared<Json::Value>();
    Json::Reader reader;
    auto ret = reader.parse(jsonstr, *root);
    if (!ret) {
        ERR("json parse error, ret[%d] jsonstr[%s]\n", ret, jsonstr.c_str());
        return nullptr;
    }
    return root;
}

string Jsoncc::JsonToString(std::shared_ptr<Json::Value>& root)
{
    Json::FastWriter fast_writer;
    string value = fast_writer.write(*root);
    int size = value.size();
    if (value[size - 1] == '\n') {
        value = value.substr(0, size - 1);
    }
    return value;
}

string Jsoncc::JsonToString(Json::Value& root)
{
    Json::FastWriter fast_writer;
    string value = fast_writer.write(root);
    int size = value.size();
    if (value[size - 1] == '\n') {
        value = value.substr(0, size - 1);
    }
    return value;
}
