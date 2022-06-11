#pragma once

#include <map>
#include <vector>
#include <iostream>
#include <any>

class Json
{
public:
    Json();
    virtual ~Json();
    std::string toString(std::any &json);
    int jsonToString(std::any &json, std::string &jsonstr);
    int jsonToString(std::map<std::string, std::any> &json, std::string &jsonstr);
    int jsonToString(std::vector<std::any> &json, std::string &jsonstr);

    int stringToJson(std::string &jsonstr, std::any &json);
    int stringToJson(std::string &jsonstr, std::any &json, int &index);
    int stringToJson(std::string &jsonstr, std::map<std::string, std::any> &json, int &index);
    int stringToJson(std::string &jsonstr, std::vector<std::any> &json, int &index);
};

