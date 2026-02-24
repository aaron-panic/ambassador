#ifndef DAMB_FORMAT_HXX_INCLUDED
#define DAMB_FORMAT_HXX_INCLUDED

#include "amb_types.hxx"
#include <vector>

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
    constexpr u16 CHUNK_HEADER_SIZE = 8;

    constexpr const char* CL_IMAGE = "IMAG";
    constexpr const char* CL_ATLAS = "ATLS";
    constexpr const char* CL_MAP_LAYER = "MAPL";
    constexpr const char* CL_AUDIO = "AUDI";
    constexpr const char* CL_STRINGS = "STRS";
    constexpr const char* CL_ENTITY = "ENTS";

    // File Header
    // ----------
    typedef struct Header {
        char magic[8] = {};
        u16 version = VERSION;
        u64 file_size = 0;
        u32 flags = 0;

        u64 toc_offset = 0;
        u32 toc_count = 0;
        u32 toc_entry_size = TOC_ENTRY_SIZE;

        u8 reserved[26] = {};
    } Header;

    // Table of Contents
    // ----------
    typedef struct TocEntry {
        char type[4] = {};
        u32 id = 0;
        u32 flags = 0;
        u32 deps_count = 0;
        
        u64 offset = 0;
        u64 size = 0;
        u64 uncompressed_size = 0;
        u32 crc32 = 0;

        u8 reserved[4] = {};
    } TocEntry;

    // Chunks
    // ----------
    typedef struct ChunkHeader {
        char type[4] = {};
        u32 id = 0;
    } ChunkHeader;

    // Image Chunk
    // ----------
    typedef struct ImageChunk {
        ChunkHeader header;
        ImageFormat format = ImageFormat::png;
        u32 width = 0;
        u32 height = 0;
        u64 size = 0;
        std::vector<u8> data;   
    } ImageChunk;

    // Atlas Chunk
    // ----------
    typedef struct AtlasRecord {
        u16 src_x = 0;
        u16 src_y = 0;
        u16 src_w = 0;
        u16 src_h = 0;

        u32 flags = 0;

        i16 anchor_x = 0;
        i16 anchor_y = 0;

        u32 name_str_offset = 0;
        u32 reserved = 0;
    } AtlasRecord;

    typedef struct AtlasChunk {
        ChunkHeader header;
        u32 image_id = 0;
        
        u32 flags = 0;
        
        u32 asset_count = 0;
        u16 record_size = sizeof(AtlasRecord);
        std::vector<AtlasRecord> records;
    } AtlasChunk;

    // Map Layer Chunk
    // ----------
    typedef struct MapCell {
        u16 atlas_id = 0;
        u16 asset_index = 0;
    } MapCell;

    typedef struct MapLayerChunk {
        ChunkHeader header;

        u32 map_width = 0;
        u32 map_height = 0;

        i32 layer_z = 0;

        u16 atlas_id = 0;
        MapEncoding encoding = MapEncoding::raw;

        u16 mapcell_stride = sizeof(MapCell);
        u16 reserved = 0;

        std::vector<MapCell> cells;
    } MapLayerChunk;

} // End namespace amb::damb

#endif