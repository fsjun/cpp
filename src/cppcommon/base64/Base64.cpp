#include "base64/Base64.h"
#include <boost/beast/core/detail/base64.hpp>

string Base64::Encode(const string& input)
{
    string output;
    std::size_t len = input.size();
    output.resize(boost::beast::detail::base64::encoded_size(len));
    output.resize(boost::beast::detail::base64::encode(&output[0], input.c_str(), len));
    return output;
}

string Base64::Decode(const string& input)
{
    string output;
    std::size_t len = input.size();
    output.resize(boost::beast::detail::base64::decoded_size(len));
    auto result = boost::beast::detail::base64::decode(&output[0], input.data(), len);
    output.resize(result.first);
    return output;
}
