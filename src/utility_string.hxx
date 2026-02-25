#ifndef UTILITY_STRING_HXX_INCLUDED
#define UTILITY_STRING_HXX_INCLUDED

#include <string>
#include <vector>

namespace amb::utility {
    std::string trim(const std::string& value);
    std::vector<std::string> split(const std::string& value, char delimiter);
    std::vector<std::string> splitWhitespace(const std::string& value);
}

#endif
