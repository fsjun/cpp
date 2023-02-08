#include "encoding/Encoding.h"
#include "iconv.h"
#include "tools/Defer.h"
#include <memory>
#include <string.h>

int Encoding::Convert(string from_code, string to_code, string src, string& dst)
{
    iconv_t h_iconv = iconv_open(to_code.c_str(), from_code.c_str());
    if (nullptr == h_iconv) {
        ERR("iconv do not support from %s to %s\n", from_code.c_str(), to_code.c_str());
        return -1;
    }
    Defer d([h_iconv]() { iconv_close(h_iconv); });
    int ret = 0;
    char* in_buf = (char*)src.c_str();
    size_t in_buf_size = src.size();
    vector<char> out_vec;
    out_vec.resize(in_buf_size);
    while (true) {
        char* out_buf = out_vec.data();
        size_t out_buf_size = out_vec.size() - 1;
        ret = iconv(h_iconv, &in_buf, &in_buf_size, &out_buf, &out_buf_size);
        if (ret < 0) {
            ret = errno;
            if (ret == E2BIG) {
                out_vec.resize(out_vec.size() * 2);
                continue;
            } else {
                ERR("%d:%s\n", ret, strerror(ret));
                return -1;
            }
        }
        break;
    }
    dst = out_vec.data();
    return 0;
}

int Encoding::Utf8ToGb18030(string src, string& dst)
{
    return Convert("UTF-8", "GB18030", src, dst);
}

int Encoding::Gb18030ToUtf8(string src, string& dst)
{
    return Convert("GB18030", "UTF-8", src, dst);
}
