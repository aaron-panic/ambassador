#ifndef DAMB_IMAG_HXX_INCLUDED
#define DAMB_IMAG_HXX_INCLUDED

#include "damb_format.hxx"

#include <type_traits>

namespace amb::damb {
    enum class ImageFormat : u8 {
        png = 1
    };

    constexpr u16 IMAG_HEADER_SIZE = 32;

    struct ImageChunkHeader {
        ChunkHeader header;
        u64 size = 0;
        u32 width = 0;
        u32 height = 0;
        ImageFormat format = ImageFormat::png;
        u8 reserved[7] = {};

    };
    static_assert(sizeof(ImageChunkHeader) == IMAG_HEADER_SIZE, "ImageChunkHeader size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<ImageChunkHeader>, "ImageChunkHeader must be POD/trivially copyable.");
}

#endif
