#pragma once

#include "tools/cpp_common.h"

class Encoding {
public:
    static int Utf8ToGb18030(string src, string& dst);
    static int Gb18030ToUtf8(string src, string& dst);
    static int Convert(string from_code, string to_code, string src, string& dst);

    static string GetSystemEncoding();
    static int Utf8ToSystemEncoding(string src, string& dst);
};
