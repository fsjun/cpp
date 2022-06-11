#include "Url.h"
#include <algorithm>
#include <cctype>
#include <string>

using std::distance;
using std::search;

map<string, int> Url::sDefaultPortMap = {
    { "http", 80 },
    { "https", 443 }
};

int Url::parse(string url_s)
{
    const string prot_end("://");
    string::iterator prot_i = search(url_s.begin(), url_s.end(), prot_end.begin(), prot_end.end());
    protocol.reserve(distance(url_s.begin(), prot_i));
    transform(url_s.begin(), prot_i, back_inserter(protocol), ::tolower);
    if (prot_i == url_s.end()) {
        return 0;
    }
    advance(prot_i, prot_end.length());
    if (prot_i == url_s.end()) {
        return 0;
    }
    string::iterator port_i = find(prot_i, url_s.end(), ':');
    string::iterator path_i;
    if (port_i != url_s.end()) {
        host.reserve(distance(prot_i, port_i));
        transform(prot_i, port_i, back_inserter(host), ::tolower); // host is icase
        ++port_i;
        if (port_i == url_s.end()) {
            return 0;
        }
        path_i = find(port_i, url_s.end(), '/');
        string port_str;
        port_str.reserve(distance(port_i, path_i));
        copy(port_i, path_i, back_inserter(port_str));
        port = std::stoi(port_str);
        if (path_i == url_s.end()) {
            return 0;
        }
    } else {
        path_i = find(prot_i, url_s.end(), '/');
        host.reserve(distance(prot_i, path_i));
        transform(prot_i, path_i, back_inserter(host), ::tolower); // host is icase
        auto it = sDefaultPortMap.find(protocol);
        if (it != sDefaultPortMap.end()) {
            port = it->second;
        }
        if (path_i == url_s.end()) {
            return 0;
        }
    }
    string path_s;
    path_s.assign(path_i, url_s.end());
    parsePath(path_s);
    return 0;
}

int Url::parsePath(string path_s)
{
    string::iterator query_i = find(path_s.begin(), path_s.end(), '?');
    path.assign(path_s.begin(), query_i);
    if (query_i == path_s.end()) {
        return 0;
    }
    query.assign(query_i, path_s.end());
    string query_s;
    query_s.assign(++query_i, path_s.end());
    parseQuery(query_s);
    return 0;
}

int Url::parseQuery(string query_s)
{
    string::iterator last_it = query_s.begin();
    string::iterator it = find(last_it, query_s.end(), '&');
    while (it != query_s.end()) {
        string str(last_it, it);
        string key;
        string value;
        int pos = str.find('=');
        if (pos == string::npos) {
            key = str;
        } else {
            key = str.substr(0, pos);
            value = str.substr(pos + 1);
        }
        mQueryMap.emplace(key, value);
        ++it;
        last_it = it;
        it = find(it, query_s.end(), '&');
    }
    if (last_it != it) {
        string str(last_it, it);
        string key;
        string value;
        int pos = str.find('=');
        if (pos == string::npos) {
            key = str;
        } else {
            key = str.substr(0, pos);
            value = str.substr(pos + 1);
        }
        mQueryMap.emplace(key, value);
    }
    return 0;
}

string Url::get(string key)
{
    auto it = mQueryMap.find(key);
    if (it == mQueryMap.end()) {
        return "";
    }
    return it->second;
}
