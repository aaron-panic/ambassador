#ifndef DAMB_FORMAT_HXX_INCLUDED
#define DAMB_FORMAT_HXX_INCLUDED

#include "amb_types.hxx"

#include <type_traits>

namespace amb::damb {
    // Indentifier Classifications
    // ----------
    enum class CompressionMethod : u8 {
        none = 0,
        zstd = 1,
    };

    enum class ImageFormat : u8 {
        png = 1
    };

    enum class MapEncoding : u8 {
        raw = 0
    };

// Constants
    // ----------
    constexpr const char* MAGIC = "DATA-AMB";
    constexpr u16 VERSION = 1;
    constexpr u16 HEADER_SIZE = 64;
    constexpr u16 TOC_ENTRY_SIZE = 48;
    constexpr u16 CHUNK_HEADER_SIZE = 6;
    constexpr u16 IMAG_HEADER_SIZE = 32;
    constexpr u16 ATLS_RECORD_SIZE = 24;
    constexpr u16 ATLS_HEADER_SIZE = 20;
    constexpr u16 MAPCELL_SIZE = 4;
    constexpr u16 MAPL_HEADER_SIZE = 28;
    
    constexpr const char* CL_IMAGE = "IMAG";
    constexpr const char* CL_ATLAS = "ATLS";
    constexpr const char* CL_MAP_LAYER = "MAPL";
    constexpr const char* CL_AUDIO = "AUDI";
    constexpr const char* CL_STRINGS = "STRS";
    constexpr const char* CL_ENTITY = "ENTS";

    // File Header
    // ----------
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

    // Table of Contents
    // ----------
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

    // Chunks
    // ----------
    typedef struct ChunkHeader {
        char type[4] = {};
        u16 id = 0;
    } ChunkHeader;
    static_assert(sizeof(ChunkHeader) == CHUNK_HEADER_SIZE, "ChunkHeader size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<ChunkHeader>, "ChunkHeader must be POD/trivially copyable.");

    // Image Chunk
    // ----------
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

    // Atlas Chunk
    // ----------
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

    // Map Layer Chunk
    // ----------
    struct MapCell {
        u16 id = 0;
        u16 atlas_record_index = 0;
    };
    static_assert(sizeof(MapCell) == MAPCELL_SIZE, "MapCell size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<MapCell>, "MapCell must be POD/trivially copyable.");

    struct MapLayerChunkHeader {
        ChunkHeader header;

        u32 width = 0;
        u32 height = 0;

        i32 reserved_layer_order = 0;

        u16 atlas_id = 0;
        MapEncoding encoding = MapEncoding::raw;

        u8 reserved[5] = {};

    };
    static_assert(sizeof(MapLayerChunkHeader) == MAPL_HEADER_SIZE, "MapLayerChunkHeader size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<MapLayerChunkHeader>, "MapLayerChunkHeader must be POD/trivially copyable.");

    constexpr u64 Align8(u64 sz) { return (sz + 7u) & ~u64{7}; }
    constexpr u64 PadTo8(u64 sz) { return Align8(sz) - sz; }

} // End namespace amb::damb

#endif
