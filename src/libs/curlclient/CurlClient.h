#pragma once
#define CURL_STATICLIB
extern "C" {
#include <curl/curl.h>
}
#include <iostream>
#include <map>
#include <any>
#include "jsonany/Json.h"
using std::string;
using std::map;
using std::any;

class HttpClient {
public:
	HttpClient();
	virtual ~HttpClient();

	int postForm(string url, int& code, string& contentType, string& content, map<string, any>* param = nullptr, map<string, string>* headers = nullptr, long timeout = 10);

	int get(string url, int& code, string& contentType, string& content, map<string, any>* param = nullptr, map<string, string>* headers = nullptr, long timeout = 10);
	int post(string url, int& code, string& contentType, string& content, map<string, any>* param = nullptr, map<string, string>* headers = nullptr, long timeout = 10);

private:
	static int WriteString(void *ptr, int size, int nmemb, void *stream);
	int request(string url, int post, long timeout, map<string, string> *headers, map<string, any> *param, int& code, string& contentType, string& content);
	struct curl_slist* genHeader(map<string, string>* headers);
	string genParam(map<string, any>* param);

private:
	CURL *mCurl;
	Json mJson;
};
