
#define THIS_MODULE "json"

#include "jsonany/Jsonany.h"
#include <cstdio>
#include <iostream>
#include <log/Log.h>
#include <sstream>
#include <string>

using namespace std;

Jsonany::Jsonany()
{
}

Jsonany::~Jsonany()
{
}

string Jsonany::toString(any& json)
{
    string jsonstr;
    std::ostringstream is;
    is << std::boolalpha;
    if (json.type() == typeid(bool)) {
        is << any_cast<bool>(json);
    }
    // signed int
    else if (json.type() == typeid(int)) {
        is << any_cast<int>(json);
    } else if (json.type() == typeid(long)) {
        is << any_cast<long>(json);
    } else if (json.type() == typeid(long long)) {
        is << any_cast<long long>(json);
    }
    // unsigned int
    else if (json.type() == typeid(unsigned int)) {
        is << any_cast<unsigned int>(json);
    } else if (json.type() == typeid(unsigned long)) {
        is << any_cast<unsigned long>(json);
    } else if (json.type() == typeid(unsigned long long)) {
        is << any_cast<unsigned long long>(json);
    } else if (json.type() == typeid(float)) {
        is << any_cast<float>(json);
    } else if (json.type() == typeid(double)) {
        is << any_cast<double>(json);
    }
    // string
    else if (json.type() == typeid(string)) {
        is << any_cast<string>(json);
    } else if (json.type() == typeid(char* const)) {
        is << any_cast<char* const>(json);
    } else if (json.type() == typeid(char const*)) {
        is << any_cast<char const*>(json);
    } else {
        ERRLN("unsupported json type[{}]", json.type().name());
        return "";
    }
    jsonstr = is.str();
    return jsonstr;
}

int Jsonany::jsonToString(any& json, string& jsonstr)
{
    int ret = 0;
    std::ostringstream is;
    is << std::boolalpha;
    if (json.type() == typeid(bool)) {
        is << any_cast<bool>(json);
    }
    // signed int
    else if (json.type() == typeid(int)) {
        is << any_cast<int>(json);
    } else if (json.type() == typeid(long)) {
        is << any_cast<long>(json);
    } else if (json.type() == typeid(long long)) {
        is << any_cast<long long>(json);
    }
    // unsigned int
    else if (json.type() == typeid(unsigned int)) {
        is << any_cast<unsigned int>(json);
    } else if (json.type() == typeid(unsigned long)) {
        is << any_cast<unsigned long>(json);
    } else if (json.type() == typeid(unsigned long long)) {
        is << any_cast<unsigned long long>(json);
    } else if (json.type() == typeid(float)) {
        is << any_cast<float>(json);
    } else if (json.type() == typeid(double)) {
        is << any_cast<double>(json);
    }
    // string
    else if (json.type() == typeid(string)) {
        is << "\"" << any_cast<string>(json) << "\"";
    } else if (json.type() == typeid(char* const)) {
        is << "\"" << any_cast<char* const>(json) << "\"";
    } else if (json.type() == typeid(char const*)) {
        is << "\"" << any_cast<char const*>(json) << "\"";
    } else if (json.type() == typeid(map<string, any>)) {
        auto value = any_cast<map<string, any>>(json);
        string str;
        ret = jsonToString(value, str);
        if (ret < 0) {
            return -1;
        }
        is << str;
    } else if (json.type() == typeid(vector<any>)) {
        auto value = any_cast<vector<any>>(json);
        string str;
        ret = jsonToString(value, str);
        if (ret < 0) {
            return ret;
        }
        is << str;
    } else {
        ERRLN("unsupported json type[{}]", json.type().name());
        return -1;
    }
    jsonstr = is.str();
    return 0;
}

int Jsonany::jsonToString(map<string, any>& json, string& jsonstr)
{
    int ret = 0;
    std::ostringstream is;
    is << std::boolalpha;
    is << "{";
    for (auto it = json.begin(); it != json.end(); it++) {
        auto key = it->first;
        auto value = it->second;
        string str;
        ret = jsonToString(value, str);
        if (ret < 0) {
            return ret;
        }
        is << "\"" << key << "\":" << str << ",";
    }
    is.seekp(-1, std::ios_base::end);
    is << "}";
    jsonstr = is.str();
    return 0;
}

int Jsonany::jsonToString(vector<any>& json, string& jsonstr)
{
    int ret = 0;
    std::ostringstream is;
    is << std::boolalpha;
    is << "[";
    for (auto it = json.begin(); it != json.end(); it++) {
        auto val = *it;
        string str;
        ret = jsonToString(val, str);
        if (ret < 0) {
            return ret;
        }
        is << str << ",";
    }
    is.seekp(-1, std::ios_base::end);
    is << "]";
    jsonstr = is.str();
    return 0;
}

int Jsonany::stringToJson(string& jsonstr, any& json)
{
    int index = 0;
    return stringToJson(jsonstr, json, index);
}

int Jsonany::stringToJson(string& jsonstr, any& json, int& index)
{
    int ret = 0;
    int size = jsonstr.size();
    while (index < size) {
        char c = jsonstr[index];
        if (c == ' ' || c == '\t' || c == '\n' || c == ',') {
            index++;
            continue;
        }
        if (c == '{') {
            index++;
            map<string, any> dict;
            ret = stringToJson(jsonstr, dict, index);
            if (ret < 0) {
                return ret;
            }
            json = dict;
            break;
        } else if (c == '[') {
            index++;
            vector<any> v;
            ret = stringToJson(jsonstr, v, index);
            if (ret < 0) {
                return ret;
            }
            json = v;
            break;
        } else if (c == '"' || c == '\'') {
            index++;
            int i = index;
            while (1) {
                i = jsonstr.find(c, i);
                if (i < 0) {
                    ERRLN("json format is error at[{}]: {} is not match", index, c);
                    return -1;
                }
                if (jsonstr[i - 1] != '\\') {
                    break;
                }
                i++;
            }
            string val = jsonstr.substr(index, i - index);
            json = val;
            i = jsonstr.find_first_of(":,]}", i);
            if (i == string::npos) {
                ERRLN("json format is error at[{}]: :,]}} is not found", i);
                return -1;
            }
            index = i;
            break;
        } else {
            int i = jsonstr.find_first_of(":,]}", index);
            if (i == string::npos) {
                ERRLN("json format is error at[{}]: :,]}} is not found", index);
                return -1;
            }
            string value = jsonstr.substr(index, i - index);
            int j = value.find('.');
            if (j == string::npos) {
                if (value.compare(0, 4, "true") == 0) {
                    bool val = true;
                    json = val;
                } else if (value.compare(0, 5, "false") == 0) {
                    bool val = false;
                    json = val;
                } else {
                    long val = stol(value, nullptr, 0);
                    json = val;
                }
            } else {
                float val = stof(value);
                json = val;
            }
            index = i;
            break;
        }
    }
    return 0;
}

int Jsonany::stringToJson(string& jsonstr, map<string, any>& json, int& index)
{
    int size = jsonstr.size();
    map<string, any> dict;
    int ret = 0;
    while (index < size) {
        char c = jsonstr[index];
        if (c == ' ' || c == '\t' || c == '\n' || c == ',') {
            index++;
            continue;
        }
        if (c == '}') {
            index++;
            break;
        }
        any value;
        int ret = stringToJson(jsonstr, value, index);
        if (ret < 0) {
            return ret;
        }
        string key;
        if (value.type() != typeid(string)) {
            ERRLN("unsupported key type[{}] at[{}]", value.type().name(), index);
            return -1;
        }
        key = any_cast<string>(value);
        if (jsonstr[index] != ':') {
            ERRLN("json format error at[{}]: it should be :", index);
            return -1;
        }
        index++;
        ret = stringToJson(jsonstr, value, index);
        if (ret < 0) {
            return ret;
        }
        dict.emplace(key, value);
    }
    json = dict;
    return 0;
}

int Jsonany::stringToJson(string& jsonstr, vector<any>& json, int& index)
{
    int size = jsonstr.size();
    vector<any> v;
    int ret = 0;
    while (index < size) {
        char c = jsonstr[index];
        if (c == ',') {
            index++;
            continue;
        }
        if (c == ']') {
            index++;
            break;
        }
        if (c == '[') {
            index++;
            vector<any> jsonv;
            ret = stringToJson(jsonstr, jsonv, index);
            if (ret < 0) {
                return ret;
            }
            v.emplace_back(jsonv);
        } else if (c == '{') {
            index++;
            map<string, any> dict;
            ret = stringToJson(jsonstr, dict, index);
            if (ret < 0) {
                return ret;
            }
            v.emplace_back(dict);
        } else {
            any value;
            ret = stringToJson(jsonstr, value, index);
            if (ret < 0) {
                return ret;
            }
            v.emplace_back(value);
        }
    }
    json = v;
    return 0;
}
