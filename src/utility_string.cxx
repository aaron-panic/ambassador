#include "utility_string.hxx"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace amb::utility {
    std::string trim(const std::string& value) {
        const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };

        const auto first = std::find_if_not(value.begin(), value.end(), is_space);
        if (first == value.end()) {
            return "";
        }

        const auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
        return std::string(first, last);
    }

    std::vector<std::string> split(const std::string& value, char delimiter) {
        std::vector<std::string> result;
        std::stringstream stream(value);
        std::string token;

        while (std::getline(stream, token, delimiter)) {
            result.push_back(token);
        }

        return result;
    }

    std::vector<std::string> splitWhitespace(const std::string& value) {
        std::istringstream stream(value);
        std::vector<std::string> result;
        std::string token;

        while (stream >> token) {
            result.push_back(token);
        }

        return result;
    }
}
