#pragma once

#include "tools/cpp_common.h"

class Base64 {
public:
    static string Encode(const string& input);
    static string Decode(const string& input);
};
