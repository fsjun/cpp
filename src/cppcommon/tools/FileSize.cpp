#include "tools/FileSize.h"
#include "log/Log.h"
#include <filesystem>

int FileSize::GetDiskSize(string path, uintmax_t capacity, uintmax_t free_size)
{
    std::error_code ec;
    std::filesystem::space_info sp = std::filesystem::space(path, ec);
    if (ec) {
        ERR("get space info error, %d:%s\n", ec.value(), ec.message().c_str());
        return -1;
    }
    capacity = sp.capacity;
    free_size = sp.available;
    return 0;
}

uintmax_t FileSize::GetDirSize(string path)
{
    if (!std::filesystem::exists(path)) {
        return 0;
    }
    if (!std::filesystem::is_directory(path)) {
        return 0;
    }
    if (std::filesystem::is_empty(path)) {
        return 0;
    }
    uintmax_t size = 0;
    for (auto& de : std::filesystem::directory_iterator(path)) {
        if (de.is_regular_file()) {
            size += de.file_size();
        } else if (de.is_directory()) {
            size += GetDirSize(de.path().generic_string());
        }
    }
    return size;
}
