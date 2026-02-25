#ifndef UTILITY_PARSE_HXX_INCLUDED
#define UTILITY_PARSE_HXX_INCLUDED

#include "amb_types.hxx"

#include <cstddef>
#include <string>
#include <utility>

namespace amb::utility {
    std::pair<std::string, std::string> parseKeyValue(const std::string& token, std::size_t line_number);

    u64 parseUnsigned(const std::string& value, std::size_t line_number, const std::string& field_name);
    i32 parseSigned32(const std::string& value, std::size_t line_number, const std::string& field_name);
    i16 parseSigned16(const std::string& value, std::size_t line_number, const std::string& field_name);
    u16 parseUnsigned16(const std::string& value, std::size_t line_number, const std::string& field_name);
    u32 parseUnsigned32(const std::string& value, std::size_t line_number, const std::string& field_name);
}

#endif
