#pragma once

#ifdef _WIN32
//define something for Windows (32-bit and 64-bit, this part is common)

#ifdef IMPORTING
#define API_EXPORT __declspec(dllimport)
#else
#define API_EXPORT __declspec(dllexport)
#endif

#ifdef _WIN64
//define something for Windows (64-bit only)
#else
//define something for Windows (32-bit only)
#endif
#elif __APPLE__

#define API_EXPORT __attribute__((visibility("default")))

#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#elif TARGET_OS_IPHONE
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#elif __ANDROID__
// android
#elif __linux__
#define API_EXPORT __attribute__((visibility("default")))
#elif __unix__ // all unices not caught above
// Unix
#elif defined(_POSIX_VERSION)
// POSIX
#else
#error "Unknown compiler"
#endif
