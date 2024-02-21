#include "CurlClient.h"
#include "log/Log.h"
#include <iostream>
#include <sstream>
using std::ostringstream;
using std::unique_ptr;

HttpClient::HttpClient()
{
    CURLcode ret;
    ret = curl_global_init(CURL_GLOBAL_ALL);
    if (ret != CURLE_OK) {
        ERR("curl_global_init() failed[{}]: {}\n", ret, curl_easy_strerror(ret));
        return;
    }
    mCurl = curl_easy_init();
}

HttpClient::~HttpClient()
{
    if (mCurl) {
        curl_easy_cleanup(mCurl);
    }
    curl_global_cleanup();
}

int HttpClient::postForm(string url, int& code, string& contentType, string& content, map<string, any>* param, map<string, string>* headers, long timeout)
{
    unique_ptr<map<string, string>> h(new map<string, string>());
    if (headers) {
        h->swap(*headers);
    }
    if (h->find("Content-Type") == h->end()) {
        h->emplace("Content-Type", "application/x-www-form-urlencoded");
    }
    return request(url, 1, timeout, h.get(), param, code, contentType, content);
}

int HttpClient::get(string url, int& code, string& contentType, string& content, map<string, any>* param, map<string, string>* headers, long timeout)
{
    return request(url, 0, timeout, headers, param, code, contentType, content);
}

int HttpClient::post(string url, int& code, string& contentType, string& content, map<string, any>* param, map<string, string>* headers, long timeout)
{
    return request(url, 1, timeout, headers, param, code, contentType, content);
}

int HttpClient::WriteString(void* ptr, int size, int nmemb, void* stream)
{
    string data((const char*)ptr, size * nmemb);
    *((ostringstream*)stream) << data;
    return size * nmemb;
}

struct curl_slist* HttpClient::genHeader(map<string, string>* headers)
{
    struct curl_slist* headerlist = nullptr;
    for (auto it : *headers) {
        ostringstream oss;
        oss << it.first << ": " << it.second;
        headerlist = curl_slist_append(headerlist, oss.str().c_str());
        if (!headerlist) {
            ERR("curl_slist_append() return nullptr maybe memory not enough\n");
            return nullptr;
        }
    }
    return headerlist;
}

string HttpClient::genParam(map<string, any>* param)
{
    ostringstream oss;
    for (auto it : *param) {
        oss << it.first << "=" << mJson.toString(it.second) << "&";
    }
    oss.seekp(-1, std::ios_base::end);
    oss << '\0';
    return oss.str();
}

int HttpClient::request(string url, int post, long timeout, map<string, string>* headers, map<string, any>* param, int& code, string& contentType, string& content)
{
    int ret = 0;
    CURLcode res;
    ostringstream oss;
    string p;
    struct curl_slist* headerlist = nullptr;
    char* ct = nullptr;
    curl_easy_setopt(mCurl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(mCurl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(mCurl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, HttpClient::WriteString);
    curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, &oss);
    if (headers) {
        headerlist = genHeader(headers);
        if (!headerlist) {
            return -1;
        }
        curl_easy_setopt(mCurl, CURLOPT_HTTPHEADER, headerlist);
    }
    if (post) {
        curl_easy_setopt(mCurl, CURLOPT_POST, 1L);
        if (param && !param->empty()) {
            p = genParam(param);
            curl_easy_setopt(mCurl, CURLOPT_POSTFIELDS, p.c_str());
        }
    } else {
        curl_easy_setopt(mCurl, CURLOPT_HTTPGET, 1L);
        if (param && !param->empty()) {
            p = genParam(param);
            url += "?" + p;
        }
    }
    curl_easy_setopt(mCurl, CURLOPT_URL, url.c_str());
    res = curl_easy_perform(mCurl);
    if (res != CURLE_OK) {
        ERR("curl_easy_perform() failed[{}]: {}\n", ret, curl_easy_strerror(res));
        ret = -1;
        goto end;
    }
    curl_easy_getinfo(mCurl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_getinfo(mCurl, CURLINFO_CONTENT_TYPE, &ct);
    if (ct) {
        contentType = ct;
    }
    content = oss.str();
    INFO("http post[{}] url[{}] timeout[{}] form[{}] code[{}] contentType[{}] content[{}]\n", post, url, timeout, p, code, contentType, content);
end:
    if (headerlist) {
        curl_slist_free_all(headerlist);
    }
    return ret;
}
