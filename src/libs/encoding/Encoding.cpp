#include "encoding/Encoding.h"
#include "boost/exception/exception.hpp"
#include "iconv.h"
#include "tools/Defer.h"
#include <boost/exception/all.hpp>
#include <boost/locale.hpp>
#include <exception>
#include <string.h>

int Encoding::Utf8ToGb18030(string src, string& dst)
{
    return Convert("UTF-8", "GB18030", src, dst);
}

int Encoding::Gb18030ToUtf8(string src, string& dst)
{
    return Convert("GB18030", "UTF-8", src, dst);
}

int Encoding::Convert(string from_code, string to_code, string src, string& dst)
{
    iconv_t h_iconv = iconv_open(to_code.c_str(), from_code.c_str());
    if (nullptr == h_iconv) {
        ERR("iconv do not support from {} to {}\n", from_code, to_code);
        return -1;
    }
    Defer d([h_iconv]() { iconv_close(h_iconv); });
    int ret = 0;
    vector<char> out_vec;
    out_vec.resize(src.size() + 1);
    while (true) {
        char* in_buf = (char*)src.c_str();
        size_t in_buf_size = src.size();
        char* out_buf = out_vec.data();
        size_t out_buf_size = out_vec.size() - 1;
        ret = iconv(h_iconv, &in_buf, &in_buf_size, &out_buf, &out_buf_size);
        if (ret < 0) {
            ret = errno;
            if (ret == E2BIG) {
                out_vec.resize(out_vec.size() * 2);
                continue;
            } else {
                ERR("{}:{}\n", ret, strerror(ret));
                return -1;
            }
        }
        break;
    }
    dst = out_vec.data();
    return 0;
}

string Encoding::GetSystemEncoding()
{
    string strCodePage = boost::locale::util::get_system_locale();
    std::locale loc = boost::locale::generator().generate(strCodePage);
    return std::use_facet<boost::locale::info>(loc).encoding();
}

int Encoding::Utf8ToSystemEncoding(string src, string& dst)
{
    int ret = -1;
    string system_encoding = GetSystemEncoding();
    try {
        dst = boost::locale::conv::between(src, system_encoding, "UTF8");
        ret = 0;
    } catch (boost::exception& e) {
        ERR("utf8 to system encoding error, exception: {}\n", boost::diagnostic_information(e));
    } catch (const std::exception& e) {
        ERR("utf8 to system encoding error, exception: {}\n", e.what());
    } catch (...) {
        ERR("utf8 to system encoding error\n");
    }
    return ret;
}
