#include "process/WatchDog.h"
#include "boost/process.hpp"

namespace bp = boost::process;

void WatchDog::addProgram(string path)
{
    INFO("start program, path:%s\n", path.c_str());
    std::error_code ec;
    bp::child c(
        path,
        *mIoContext,
        ec,
        bp::std_out.close(),
        bp::std_err.close(),
        bp::std_in < bp::null,
        bp::on_exit([path, this](int exit, const std::error_code& ec_in) {
            ERR("%s exit, exit:%d ec:%d\n", path.c_str(), exit, ec_in.value());
            std::this_thread::sleep_for(std::chrono::seconds(mIntervalSecond));
            addProgram(path);
        }));
    c.detach();
}

void WatchDog::run()
{
    mIoContext->run();
}
