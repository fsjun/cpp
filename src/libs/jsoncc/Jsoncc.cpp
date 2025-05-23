
#include "Jsoncc.h"

std::shared_ptr<Json::Value> Jsoncc::StringToJson(string jsonstr)
{
    auto root = std::make_shared<Json::Value>();
    Json::Reader reader;
    auto ret = reader.parse(jsonstr, *root);
    if (!ret) {
        ERRLN("json parse error, ret[{}] jsonstr[{}]", ret, jsonstr);
        return nullptr;
    }
    return root;
}

string JsonToString(std::shared_ptr<Json::Value>& root)
{
    Json::FastWriter fast_writer;
    string value = fast_writer.write(*root);
    int size = value.size();
    if (value[size - 1] == '\n') {
        value = value.substr(0, size - 1);
    }
    return value;
}

string JsonToString(Json::Value& root)
{
    Json::FastWriter fast_writer;
    string value = fast_writer.write(root);
    int size = value.size();
    if (value[size - 1] == '\n') {
        value = value.substr(0, size - 1);
    }
    return value;
}

string Jsoncc::JsonToString(std::shared_ptr<Json::Value>& root)
{
    return JsonToString(*root);
}

string Jsoncc::JsonToString(Json::Value& root)
{
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder.settings_["emitUTF8"] = true;
    writerBuilder.settings_["indentation"] = "";
    string str = Json::writeString(writerBuilder, root);
    return str;
}
