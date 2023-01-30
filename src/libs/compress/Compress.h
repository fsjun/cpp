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
    static int UnZip7z(string zipFileName, string dir);
    /*
     7-Zip 19.00 (x64) : Copyright (c) 1999-2018 Igor Pavlov : 2019-02-21

Scanning the drive for archives:
1 file, 11937568731 bytes (12 GiB)

Listing archive: C:\Users\lxj\Downloads\b864c099-11da-4451-9ac5-6bd86369d1de.zip

--
Path = C:\Users\lxj\Downloads\b864c099-11da-4451-9ac5-6bd86369d1de.zip
Type = zip
Physical Size = 11937568731
64-bit = +
Characteristics = Zip64

   Date      Time    Attr         Size   Compressed  Name
------------------- ----- ------------ ------------  ------------------------
2022-12-23 16:05:13 D....            0            0  clone1
2022-12-19 16:17:49 ....A         8684         1862  clone1\clone1.nvram
2022-10-28 16:30:02 ....A            0            0  clone1\clone1.vmsd
2022-12-20 08:35:47 ....A         2753          988  clone1\clone1.vmx
2022-10-28 16:40:05 ....A          261          181  clone1\clone1.vmxf




7-Zip 19.00 (x64) : Copyright (c) 1999-2018 Igor Pavlov : 2019-02-21

Scanning the drive for archives:
1 file, 1696737254 bytes (1619 MiB)

Listing archive: E:\workspace\vm\vmclient\out\build\x64-Debug\download\ff5b4425-908f-4bec-870e-332cd0c4140a.zip

--
Path = E:\workspace\vm\vmclient\out\build\x64-Debug\download\ff5b4425-908f-4bec-870e-332cd0c4140a.zip
Type = zip
Physical Size = 1696737254

   Date      Time    Attr         Size   Compressed  Name
------------------- ----- ------------ ------------  ------------------------
2023-01-29 11:48:40 D....            0            0  test3
2023-01-29 11:48:39 ....A      1638400        35188  test3\test3_0129_CentOS 7-000001.vmdk
2023-01-29 11:48:39 ....A   2147483648    889691560  test3\test3_0129_CentOS 7-Snapshot1.vmem
2023-01-29 11:48:39 ....A    135748482       775803  test3\test3_0129_CentOS 7-Snapshot1.vmsn
2023-01-29 11:47:38 ....A         8684         1854  test3\test3_0129_CentOS 7.nvram
2023-01-29 11:47:39 ....A   1800667136    806196764  test3\test3_0129_CentOS 7.vmdk
2023-01-29 11:47:39 ....A          446          204  test3\test3_0129_CentOS 7.vmsd
2023-01-29 11:48:40 ....A         2662          948  test3\test3_0129_CentOS 7.vmx
     * */
    // just for zip64 large zip file
    static int GetFirstNodeName7z(string zipFileName, string& fileName);
};
