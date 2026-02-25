#ifndef UTILITY_BINARY_HXX_INCLUDED
#define UTILITY_BINARY_HXX_INCLUDED

#include "amb_types.hxx"
#include "config.hxx"

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace amb::utility {
    template <typename T>
    T readPod(std::ifstream& stream, const std::string& context) {
        static_assert(std::is_trivially_copyable_v<T>, "readPod requires trivially copyable types.");

        T value {};
        stream.read(reinterpret_cast<char*>(&value), sizeof(T));
        if (!stream) {
            throw std::runtime_error("Failed to read " + context + ".");
        }

        return value;
    }

    template <typename T>
    void appendPod(std::vector<u8>& out, const T& pod) {
        static_assert(std::is_trivially_copyable_v<T>, "appendPod requires trivially copyable types.");

        const auto* begin = reinterpret_cast<const u8*>(&pod);
        out.insert(out.end(), begin, begin + sizeof(T));
    }

    inline bool chunkTypeEquals(const char (&type)[4], const char* expected) noexcept {
        return std::memcmp(type, expected, amb::data::CHUNK_TYPE_LENGTH) == 0;
    }
}

#endif
