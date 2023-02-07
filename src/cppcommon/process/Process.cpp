#include "process/Process.h"
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
        result = vec.data();
    }
    return ret;
}
