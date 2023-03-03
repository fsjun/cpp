#pragma once

#include "tools/cpp_common.h"

class FileSize {
public:
    static int GetDiskSize(string path, uintmax_t capacity, uintmax_t free_size);
    static uintmax_t GetDirSize(string path);
};
