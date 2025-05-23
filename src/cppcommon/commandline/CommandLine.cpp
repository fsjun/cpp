
#define THIS_MODULE "CommandLine"

#include "CommandLine.h"
#include "log/Log.h"
#include <sstream>
#include <vector>

using namespace boost::program_options;

int CommandLine::parse(int argc, char* argv[], boost::program_options::options_description opts)
{
    int ret = 0;
    try {
        store(parse_command_line(argc, argv, opts), mVm);
    } catch (boost::program_options::error_with_no_option_name& ex) {
        ERRLN("parse arguments error: {}\n", ex.what());
        ret = -1;
    }
    notify(mVm);
    return ret;
}

string CommandLine::operator[](string key)
{
    auto it = mVm.find(key);
    if (it == mVm.end()) {
        return "";
    }
    return it->second.as<string>();
}

bool CommandLine::isExist(string key)
{
    return mVm.count(key) == 1;
}
