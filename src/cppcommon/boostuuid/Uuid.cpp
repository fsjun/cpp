#include "Uuid.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstdio>
#include <sstream>

using std::ostringstream;

string Uuid::Gen()
{
    boost::uuids::random_generator rgen;
    boost::uuids::uuid ranUUID = rgen();
    ostringstream oss;
    oss << ranUUID;
    return oss.str();
}
