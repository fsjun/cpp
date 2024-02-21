#pragma once

#include "log/Log.h"
#include <algorithm>
#include <any>
#include <chrono>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <vector>

using std::any;
using std::any_cast;
using std::cout;
using std::deque;
using std::endl;
using std::function;
using std::make_shared;
using std::map;
using std::multimap;
using std::mutex;
using std::ostringstream;
using std::queue;
using std::shared_ptr;
using std::string;
using std::thread;
using std::uint8_t;
using std::unique_ptr;
using std::vector;
using std::weak_ptr;

#define SAFE_END(ret, format, ...)  \
    if (ret < 0) {                  \
        ERR(format, ##__VA_ARGS__); \
        goto end;                   \
    }

#define SAFE_RETURN(ret, format, ...) \
    if (ret < 0) {                    \
        ERR(format, ##__VA_ARGS__);   \
        return ret;                   \
    }

#define SAFE_NO_RETURN(ret, format, ...) \
    if (ret < 0) {                       \
        ERR(format, ##__VA_ARGS__);      \
        return;                          \
    }
