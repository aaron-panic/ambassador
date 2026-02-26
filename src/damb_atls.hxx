#ifndef DAMB_ATLS_HXX_INCLUDED
#define DAMB_ATLS_HXX_INCLUDED

#include "damb_format.hxx"

#include <type_traits>

namespace amb::damb {
    constexpr u16 ATLS_RECORD_SIZE = 24;
    constexpr u16 ATLS_HEADER_SIZE = 20;

    struct AtlasRecord {
        u16 id = 0;
        u16 src_x = 0;
        u16 src_y = 0;
        u16 src_w = 0;
        u16 src_h = 0;

        u32 flags = 0;

        i16 anchor_x = 0;
        i16 anchor_y = 0;

        u32 name_str_offset = 0;
    };
    static_assert(sizeof(AtlasRecord) == ATLS_RECORD_SIZE, "AtlasRecord size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<AtlasRecord>, "AtlasRecord must be POD/trivially copyable.");

    struct AtlasChunkHeader {
        ChunkHeader header;
        u32 flags = 0;
        u32 asset_count = 0;
        u16 image_id = 0;
    };
    static_assert(sizeof(AtlasChunkHeader) == ATLS_HEADER_SIZE, "AtlasChunkHeader size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<AtlasChunkHeader>, "AtlasChunkHeader must be POD/trivially copyable.");
}

#endif
