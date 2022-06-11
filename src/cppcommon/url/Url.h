#pragma once

#include <iostream>
#include <map>
#include <memory>

using std::map;
using std::string;

class Url {
public:
    int parse(string url_s);

    int parsePath(string path_s);
    int parseQuery(string query_s);
    string get(string key);

public:
    string protocol;
    string host;
    int port = 0;
    string path;
    string query;

private:
    map<string, string> mQueryMap;
    static map<string, int> sDefaultPortMap;
};
