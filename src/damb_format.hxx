#ifndef FORMAT_DAMB_HXX_INCLUDED
#define FORMAT_DAMB_HXX_INCLUDED

#include "amb_types.hxx"

#include <type_traits>

namespace amb::damb {
    constexpr const char* MAGIC = "DATA-AMB";
    constexpr u16 VERSION = 1;
    constexpr u16 HEADER_SIZE = 64;
    constexpr u16 TOC_ENTRY_SIZE = 48;
    constexpr u16 CHUNK_HEADER_SIZE = 6;

    constexpr const char* CL_IMAGE = "IMAG";
    constexpr const char* CL_ATLAS = "ATLS";
    constexpr const char* CL_MAP_LAYER = "MAPL";
    constexpr const char* CL_AUDIO = "AUDI";
    constexpr const char* CL_STRINGS = "STRS";
    constexpr const char* CL_ENTITY = "ENTS";

    struct Header {
        char magic[8] = {};
        u64 file_size = 0;
        u64 toc_offset = 0;
        u32 toc_count = 0;
        u32 toc_entry_size = TOC_ENTRY_SIZE;
        u32 flags = 0;
        u16 version = VERSION;
        u8 reserved[26] = {};
    };
    static_assert(sizeof(Header) == HEADER_SIZE, "Header size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<Header>, "Header must be POD/trivially copyable.");

    struct TocEntry {
        u64 offset = 0;
        u64 size = 0;
        u64 uncompressed_size = 0;
        u16 id = 0;
        u32 flags = 0;
        u32 deps_count = 0;
        u32 crc32 = 0;
        char type[4] = {};

        u8 reserved[4] = {};
    };
    static_assert(sizeof(TocEntry) == TOC_ENTRY_SIZE, "TOC size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<TocEntry>, "TocEntry must be POD/trivially copyable.");

    typedef struct ChunkHeader {
        char type[4] = {};
        u16 id = 0;
    } ChunkHeader;
    static_assert(sizeof(ChunkHeader) == CHUNK_HEADER_SIZE, "ChunkHeader size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<ChunkHeader>, "ChunkHeader must be POD/trivially copyable.");

    constexpr u64 Align8(u64 sz) { return (sz + 7u) & ~u64{7}; }
    constexpr u64 PadTo8(u64 sz) { return Align8(sz) - sz; }

} // namespace amb::damb

#endif
