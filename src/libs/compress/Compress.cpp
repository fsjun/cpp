#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "minizip/unzip.h"
#include "minizip/zip.h"

#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "process/Process.h"

#include "compress/Compress.h"
#include "log/Log.h"
#include "tools/Defer.h"
#include <filesystem>

int AddFileToZip(zipFile zFile, const string& fileName, const string& fileNameInZip)
{
    int opt_compress_level = Z_DEFAULT_COMPRESSION;
    zip_fileinfo zFileInfo = { 0 };
    int ret = zipOpenNewFileInZip3_64(zFile, fileNameInZip.c_str(), &zFileInfo, NULL, 0, NULL, 0, NULL, (opt_compress_level != 0) ? Z_DEFLATED : 0, opt_compress_level, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 1);
    if (ret != ZIP_OK) {
        ERRLN("zipOpenNewFileInZip3_64 failed");
        return -1;
    }
    DEBUGLN("begin write file, fileName:{} fileNameInZip:{}", fileName, fileNameInZip);
    std::fstream ifs(fileName.c_str(), std::ios::binary | std::ios::in);
    char buff[1024] = { 0 };
    int size = sizeof(buff);
    int gcount = 0;
    do {
        ifs.read(buff, size);
        gcount = ifs.gcount();
        if (gcount > 0) {
            ret = zipWriteInFileInZip(zFile, buff, gcount);
            if (ZIP_OK != ret) {
                ERRLN("zipWriteInFileInZip error, ret:{}", ret);
                ret = -1;
                break;
            }
        }
    } while (gcount == size);
    ifs.close();
    DEBUGLN("end write file, fileName:{} fileNameInZip:{}", fileName, fileNameInZip);
    int res = zipCloseFileInZip(zFile);
    if (ZIP_OK != res) {
        ERRLN("zipCloseFileInZip error, res:{}", res);
        return -1;
    }
    return ret;
}

int AddDirToZip(zipFile zFile, const string& fileNameInZip)
{
    int opt_compress_level = Z_DEFAULT_COMPRESSION;
    zip_fileinfo zFileInfo = { 0 };
    string dirNameInZip = fileNameInZip + "/";
    int ret = zipOpenNewFileInZip3_64(zFile, dirNameInZip.c_str(), &zFileInfo, NULL, 0, NULL, 0, NULL, (opt_compress_level != 0) ? Z_DEFLATED : 0, opt_compress_level, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 1);
    if (ret != ZIP_OK) {
        ERRLN("zipOpenNewFileInZip3_64 failed");
        return -1;
    }
    int res = zipCloseFileInZip(zFile);
    if (ZIP_OK != res) {
        ERRLN("zipCloseFileInZip error, res:{}", res);
        return -1;
    }
    return ret;
}

void Compress::EnumDirFiles(string dir, vector<string>& fileVec)
{
    if (!std::filesystem::is_directory(dir)) {
        return;
    }
    if (std::filesystem::is_empty(dir)) {
        fileVec.emplace_back(dir);
        return;
    }
    for (auto& de : std::filesystem::directory_iterator(dir)) {
        if (de.is_regular_file()) {
            fileVec.emplace_back(de.path().generic_string());
        } else if (de.is_directory()) {
            EnumDirFiles(de.path().generic_string(), fileVec);
        }
    }
}

int Compress::Zip(string dir, string zipFileName)
{
    vector<string> fileVec;
    EnumDirFiles(dir, fileVec);
    if (fileVec.empty()) {
        ERRLN("{} has no file", dir);
        return -1;
    }
    zipFile zFile = zipOpen64(zipFileName.c_str(), APPEND_STATUS_CREATE);
    if (zFile == NULL) {
        ERRLN("zipOpen failed");
        return -1;
    }
    Defer d([zFile]() {
        int ret = zipClose(zFile, NULL);
        if (ZIP_OK != ret) {
            ERRLN("zipClose error, ret:{}", ret);
        }
    });
    int ret = 0;
    std::filesystem::path p(dir);
    int pos = p.parent_path().generic_string().size();
    for (string fileName : fileVec) {
        string fileNameInZip;
        if (pos != 0) {
            fileNameInZip = fileName.substr(pos + 1);
        } else {
            fileNameInZip = fileName;
        }
        if (std::filesystem::is_directory(fileName)) {
            ret = AddDirToZip(zFile, fileNameInZip);
            if (ret != ZIP_OK) {
                ERRLN("write in zip failed");
                return -1;
            }
        } else {
            ret = AddFileToZip(zFile, fileName, fileNameInZip);
            if (ret != ZIP_OK) {
                ERRLN("write in zip failed");
                return -1;
            }
        }
    }
    return 0;
}

int Compress::UnZip(string zipFileName, string dir)
{
    int ret = 0;
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
    unzFile unzfile = unzOpen64(zipFileName.c_str());
    if (unzfile == NULL) {
        ERRLN("unzOpen64 failed, fileName:{}", zipFileName);
        return -1;
    }
    Defer d([unzfile]() {
        unzClose(unzfile);
    });
    unz_global_info64* globalInfo = new unz_global_info64();
    ret = unzGetGlobalInfo64(unzfile, globalInfo);
    if (ret != UNZ_OK) {
        ERRLN("unzGetGlobalInfo64 failed");
        return -1;
    }
    unz_file_info64* fileInfo = new unz_file_info64();
    char fileNameInZip[1024] = { 0 };
    int size;
    for (int i = 0; i < (int)globalInfo->number_entry; i++) {
        size = sizeof(fileNameInZip);
        ret = unzGetCurrentFileInfo64(unzfile, fileInfo, fileNameInZip, size, nullptr, 0, nullptr, 0);
        if (ret != UNZ_OK) {
            ERRLN("unzGetCurrentFileInfo64 failed");
            return -1;
        }
        string fileName = fileNameInZip;
        if (fileName.back() == '/') {
            std::filesystem::create_directories(dir + "/" + fileName);
        } else {
            std::filesystem::path p(dir + "/" + fileName);
            std::filesystem::create_directories(p.parent_path());
            ret = unzOpenCurrentFile(unzfile);
            if (ret != UNZ_OK) {
                ERRLN("unzOpenCurrentFile failed.");
                return -1;
            }
            std::ofstream ofs(p, std::ios::binary);
            char buff[1024];
            size = sizeof(buff);
            while (true) {
                memset(buff, 0, size);
                ret = unzReadCurrentFile(unzfile, buff, size);
                if (ret < 0) {
                    ERRLN("unzReadCurrentFile failed");
                    break;
                } else if (ret == 0) {
                    break;
                }
                ofs.write(buff, ret);
            }
            ofs.close();
            unzCloseCurrentFile(unzfile);
        }
        unzGoToNextFile(unzfile);
    }
    return 0;
}

int Compress::GetFirstNodeName(string zipFileName, string& fileName)
{
    int ret = 0;
    unzFile unzfile = unzOpen64(zipFileName.c_str());
    if (unzfile == NULL) {
        ERRLN("unzOpen64 failed, fileName:{}", zipFileName);
        return -1;
    }
    Defer d([unzfile]() {
        unzClose(unzfile);
    });
    unz_global_info64* globalInfo = new unz_global_info64();
    ret = unzGetGlobalInfo64(unzfile, globalInfo);
    if (ret != UNZ_OK) {
        ERRLN("unzGetGlobalInfo64 failed");
        return -1;
    }
    unz_file_info64* fileInfo = new unz_file_info64();
    char fileNameInZip[1024] = { 0 };
    int size = sizeof(fileNameInZip);
    ret = unzGetCurrentFileInfo64(unzfile, fileInfo, fileNameInZip, size, nullptr, 0, nullptr, 0);
    if (ret != UNZ_OK) {
        ERRLN("unzGetCurrentFileInfo64 failed");
        return -1;
    }
    string tmpFileName = fileNameInZip;
    int pos = tmpFileName.find_first_of('/');
    if (pos == string::npos) {
        fileName = tmpFileName;
    } else {
        fileName = tmpFileName.substr(0, pos);
    }
    return 0;
}

int Compress::Zip7z(string dir, string zipFileName)
{
    string cmd = boost::str(boost::format("7z a -tzip -mmt -mx1 \"%s\" \"%s\"") % zipFileName % dir);
    string result;
    int ret = Process::SystemGb18030(cmd, result);
    if (ret < 0) {
        ERRLN("cmd execute fail, cmd:{} result:{}", cmd, result);
        return ret;
    }
    DEBUGLN("{}{}", cmd, result);
    return ret;
}

int Compress::UnZip7z(string zipFileName, string dir)
{
    string cmd = boost::str(boost::format("7z x \"%s\" -o\"%s\"") % zipFileName % dir);
    string result;
    int ret = Process::SystemGb18030(cmd, result);
    if (ret < 0) {
        ERRLN("cmd execute fail, cmd:{} result:{}", cmd, result);
        return ret;
    }
    DEBUGLN("{}{}", cmd, result);
    return 0;
}

int Compress::GetFirstNodeName7z(string zipFileName, string& fileName)
{
    string cmd = boost::str(boost::format("7z l \"%s\"") % zipFileName);
    string result;
    int ret = Process::System(cmd, result);
    if (ret < 0) {
        ERRLN("cmd execute fail, cmd:{} result:{}", cmd, result);
        return ret;
    }
    DEBUGLN("{}{}", cmd, result);
    vector<string> lineVec;
    boost::split(lineVec, result, boost::is_any_of("\r\n"), boost::token_compress_on);
    if (lineVec.size() < 14) {
        ERRLN("result has no file, cmd:{} result:{}", cmd, result);
        return -1;
    }
    string fileStr = lineVec[13];
    vector<string> fileVec;
    boost::split(fileVec, fileStr, boost::is_any_of(" "), boost::token_compress_on);
    if (fileStr.size() < 1) {
        ERRLN("fileStr has no file path, fileStr:{}", fileStr);
        return -1;
    }
    fileName = fileVec.back();
    return ret;
}
