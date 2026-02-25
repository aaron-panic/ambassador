#include "utility_parse.hxx"

#include <limits>
#include <stdexcept>

namespace amb::utility {
    namespace {
        [[noreturn]] void throwInvalidInteger(std::size_t line_number, const std::string& field_name, const std::string& kind) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": invalid " + kind + " integer for " + field_name + "."
            );
        }
    }

    std::pair<std::string, std::string> parseKeyValue(const std::string& token, std::size_t line_number) {
        const std::size_t separator = token.find('=');
        if (separator == std::string::npos || separator == 0 || separator == token.size() - 1) {
            throw std::runtime_error("Line " + std::to_string(line_number) + ": expected key=value token.");
        }

        return {token.substr(0, separator), token.substr(separator + 1)};
    }

    u64 parseUnsigned(const std::string& value, std::size_t line_number, const std::string& field_name) {
        try {
            std::size_t consumed = 0;
            const unsigned long long parsed = std::stoull(value, &consumed, 10);
            if (consumed != value.size()) {
                throw std::runtime_error("invalid trailing characters");
            }

            return static_cast<u64>(parsed);
        } catch (const std::exception&) {
            throwInvalidInteger(line_number, field_name, "unsigned");
        }
    }

    i32 parseSigned32(const std::string& value, std::size_t line_number, const std::string& field_name) {
        try {
            std::size_t consumed = 0;
            const long parsed = std::stol(value, &consumed, 10);
            if (consumed != value.size()) {
                throw std::runtime_error("invalid trailing characters");
            }
            if (parsed < static_cast<long>(std::numeric_limits<i32>::min()) ||
                parsed > static_cast<long>(std::numeric_limits<i32>::max())) {
                throw std::runtime_error("range error");
            }
            return static_cast<i32>(parsed);
        } catch (const std::exception&) {
            throwInvalidInteger(line_number, field_name, "signed");
        }
    }

    i16 parseSigned16(const std::string& value, std::size_t line_number, const std::string& field_name) {
        const i32 parsed = parseSigned32(value, line_number, field_name);
        if (parsed < std::numeric_limits<i16>::min() || parsed > std::numeric_limits<i16>::max()) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": value out of range for i16 field " + field_name + "."
            );
        }

        return static_cast<i16>(parsed);
    }

    u16 parseUnsigned16(const std::string& value, std::size_t line_number, const std::string& field_name) {
        const u64 parsed = parseUnsigned(value, line_number, field_name);
        if (parsed > std::numeric_limits<u16>::max()) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": value out of range for u16 field " + field_name + "."
            );
        }

        return static_cast<u16>(parsed);
    }

    u32 parseUnsigned32(const std::string& value, std::size_t line_number, const std::string& field_name) {
        const u64 parsed = parseUnsigned(value, line_number, field_name);
        if (parsed > std::numeric_limits<u32>::max()) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": value out of range for u32 field " + field_name + "."
            );
        }

        return static_cast<u32>(parsed);
    }
}
