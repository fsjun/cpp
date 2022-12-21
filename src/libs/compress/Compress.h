#pragma once

#include "tools/cpp_common.h"
#include "gtest/gtest.h"

class Compress {
public:
    static void EnumDirFiles(string dir, vector<string>& fileVec);
    static int Zip(string dir, string zipFileName);
    static int UnZip(string zipFileName, string dir);
    static int GetFirstNodeName(string zipFileName, string& fileName);

    static int Zip7z(string dir, string zipFileName);
};
