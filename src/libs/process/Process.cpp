#include "process/Process.h"
#include "encoding/Encoding.h"
#include "log/Log.h"

#include <boost/process.hpp>
namespace bp = boost::process;

int Process::System(string cmd, string& result)
{
    std::future<std::vector<char>> in_buf;
    std::error_code ec;
    int ret = bp::system(
        cmd,
        ec,
        (bp::std_out & bp::std_err) > in_buf,
        bp::std_in < bp::null);
    if (ret < 0) {
        ERR("cmd:%s ret:%d %d:%s\n", cmd.c_str(), ret, ec.value(), ec.message().c_str());
        return ret;
    }
    auto vec = in_buf.get();
    if (vec.size() > 0) {
        result = string(vec.data(), vec.size());
    }
    return ret;
}

int Process::SystemGb18030(string cmd, string& result)
{
    std::future<std::vector<char>> in_buf;
    std::error_code ec;
    string dst;
    int ret = 0;
    ret = Encoding::Utf8ToGb18030(cmd, dst);
    if (ret < 0) {
        ERR("convert error from utf8 to gb18030, cmd:%s\n", cmd.c_str());
        return -1;
    }
    ret = bp::system(
        dst,
        ec,
        (bp::std_out & bp::std_err) > in_buf,
        bp::std_in < bp::null);
    if (ret < 0) {
        ERR("cmd:%s ret:%d %d:%s\n", cmd.c_str(), ret, ec.value(), ec.message().c_str());
        return ret;
    }
    auto vec = in_buf.get();
    if (vec.size() > 0) {
        string in_buf_str = string(vec.data(), vec.size());
        Encoding::Gb18030ToUtf8(in_buf_str, result);
    }
    return ret;
}
