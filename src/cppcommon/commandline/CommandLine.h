#pragma once
#include <iostream>
#include <map>
#include <boost/program_options.hpp>

using std::string;
using std::map;

class CommandLine
{
public:
    int parse(int argc, char *argv[], boost::program_options::options_description opts);
    string operator[](string key);
    bool isExist(string key);

private:
    boost::program_options::variables_map mVm;
};