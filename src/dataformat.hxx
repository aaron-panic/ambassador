#ifndef DATAAMB_FORMAT_HXX_INCLUDED
#define DATAAMB_FORMAT_HXX_INCLUDED

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>

#include <SDL3/SDL.h>

// basic types + ids
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i16 = std::int16_t;
using i32 = std::int32_t;

struct ImageId { u32 value = 0; };
struct AtlasId { u32 value = 0; };          // TOC / chunk ids (u32)
struct LayerId { u32 value = 0; };
struct AudioId { u32 value = 0; };
struct StringsId { u32 value = 0; };
struct EntitiesId { u32 value = 0; };

// Map-layer cells store atlas id as u16 for compactness:
struct AtlasId16 { u16 value = 0; };


// -----------------------------------------------------------------------------
// Container header + TOC
// -----------------------------------------------------------------------------
struct DataAmbHeader {
    char magic[8] = {};          // "DATA-AMB"
    u16  version = 1;            // v1
    u16  header_size = 64;       // fixed for v1
    u32  container_flags = 0;

    u64  toc_offset = 0;
    u32  toc_count = 0;
    u32  toc_entry_size = 48;

    u64  file_size = 0;
    u8   reserved[24] = {};
};

enum class CompressionMethod : u8 {
    None = 0,
    Zstd = 1,
};

struct TocFlags {
    // v1: interpret these fields from the raw flags u32 if you want
    CompressionMethod compression = CompressionMethod::None;
    u8  compression_level = 0; // optional preset
    u16 reserved = 0;
};

struct TocEntry {
    char type[4] = {};     // "IMAG", "ATLS", "MAPL", "AUDI", "STRS", "ENTS"
    u32  id = 0;           // id within that type
    u32  flags = 0;        // includes compression bits
    u32  deps_count = 0;   // v1: keep 0

    u64  offset = 0;
    u64  size = 0;                 // compressed or not
    u64  uncompressed_size = 0;    // 0 if not compressed
    u32  crc32 = 0;                // optional
    u32  reserved = 0;
};


// -----------------------------------------------------------------------------
// Common chunk header (stored inside each chunk payload)
// -----------------------------------------------------------------------------
struct ChunkHeader {
    char type[4] = {};
    u32  id = 0;

    u16  chunk_version = 1;   // per-chunk-type version
    u16  header_size = 16;    // bytes in this header
    u32  reserved = 0;
};


// -----------------------------------------------------------------------------
// STRS chunk: string table (file data)
// -----------------------------------------------------------------------------
struct StringsChunk {
    ChunkHeader hdr;                 // type="STRS"
    u64 string_data_size = 0;
    std::vector<u8> string_data;     // null-terminated utf-8 strings

    // runtime convenience: no function bodies requested
};


// -----------------------------------------------------------------------------
// IMAG chunk: raw PNG blob (file data)
// -----------------------------------------------------------------------------
enum class ImageFormat : u32 {
    PNG = 1,
    // reserve others
};

struct ImageChunk {
    ChunkHeader hdr;                 // type="IMAG"
    ImageFormat format = ImageFormat::PNG;
    u32 width = 0;
    u32 height = 0;

    u64 data_size = 0;
    std::vector<u8> data;            // PNG bytes
};


// -----------------------------------------------------------------------------
// ATLS chunk: atlas records (file data)
// -----------------------------------------------------------------------------
struct AtlasRecordV1 {
    u16 src_x = 0;
    u16 src_y = 0;
    u16 src_w = 0;
    u16 src_h = 0;

    u32 flags = 0;

    i16 anchor_x = 0;
    i16 anchor_y = 0;

    u32 name_str_offset = 0;   // 0 = unnamed, else offset into STRS
    u32 reserved = 0;
};

struct AtlasChunk {
    ChunkHeader hdr;                 // type="ATLS"
    ImageId image_id{};              // references IMAG id

    u32 asset_count = 0;
    u16 record_stride = sizeof(AtlasRecordV1);
    u16 atlas_flags = 0;             // reserved

    // v1 uses record vector; stride retained for forward compatibility.
    std::vector<AtlasRecordV1> records;
};


// -----------------------------------------------------------------------------
// MAPL chunk: map layer (file data)
// -----------------------------------------------------------------------------
enum class MapEncoding : u32 {
    Raw = 0,
    // Rle = 1, // future if you want
};

struct MapCellV1 {
    AtlasId16 atlas_id{0};   // compact atlas id; you agreed u16 is fine here
    u16 asset_index = 0;     // index into that atlas
};

struct MapLayerChunk {
    ChunkHeader hdr;             // type="MAPL"

    u32 map_width = 0;
    u32 map_height = 0;

    i32 layer_z = 0;

    AtlasId default_atlas_id{};  // convenience; 0 = none
    MapEncoding encoding = MapEncoding::Raw;

    u16 cell_stride = sizeof(MapCellV1);
    u16 reserved = 0;

    std::vector<MapCellV1> cells; // size = map_width*map_height, row-major
};


// -----------------------------------------------------------------------------
// ENTS chunk: entity placement (file data)
// -----------------------------------------------------------------------------
struct EntityRecordV1 {
    u32 type_str_offset = 0; // offset into STRS

    i32 x = 0;               // pixels in world space (recommended)
    i32 y = 0;

    LayerId layer_id{};      // optional association; 0 = none

    AtlasId16 atlas_id{0};   // compact reference for rendering (optional)
    u16 asset_index = 0;

    u32 flags = 0;

    i32 param0 = 0;
    i32 param1 = 0;
    i32 param2 = 0;
    i32 param3 = 0;
};

struct EntitiesChunk {
    ChunkHeader hdr;                 // type="ENTS"

    u32 entity_count = 0;
    u16 record_stride = sizeof(EntityRecordV1);
    u16 reserved = 0;

    std::vector<EntityRecordV1> records;
};


// -----------------------------------------------------------------------------
// AUDI chunk: audio blob (file data)
// -----------------------------------------------------------------------------
enum class AudioFormat : u32 {
    OGG = 1,
    // reserve others
};

struct AudioChunk {
    ChunkHeader hdr;                 // type="AUDI"
    AudioFormat format = AudioFormat::OGG;

    u64 data_size = 0;
    std::vector<u8> data;            // raw ogg bytes
};


// -----------------------------------------------------------------------------
// Runtime representation (what your game actually uses)
//   - File chunks -> these runtime objects.
//   - Note: no loading logic implemented here.
// -----------------------------------------------------------------------------

// RAII for SDL_Texture*
struct TextureDeleter {
    void operator()(SDL_Texture* t) const { if (t) SDL_DestroyTexture(t); }
};
using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;

struct RuntimeImage {
    ImageId id{};
    u32 width = 0;
    u32 height = 0;

    TexturePtr texture;      // created from ImageChunk::data (elsewhere)
};

struct RuntimeAtlas {
    AtlasId id{};
    ImageId image_id{};

    // Contiguous rects & flags for cache-friendly access:
    std::vector<SDL_FRect> src_rects; // derived from AtlasRecordV1 (or stored directly)
    std::vector<u32> flags;           // per-asset flags
    // Optional:
    std::vector<u32> name_str_offset; // offsets into STRS for tooling/debug
};

struct RuntimeLayer {
    LayerId id{};
    u32 width = 0;
    u32 height = 0;
    i32 reserved_layer_order = 0;

    // compact per-cell references:
    std::vector<MapCellV1> cells; // row-major
};

struct RuntimeEntities {
    EntitiesId id{};
    std::vector<EntityRecordV1> records;
};

struct RuntimeAudio {
    AudioId id{};
    AudioFormat format = AudioFormat::OGG;
    std::vector<u8> data; // or decoded handle in your audio system later
};

struct RuntimeStrings {
    StringsId id{};
    std::vector<u8> data; // null-terminated table
};


// -----------------------------------------------------------------------------
// ScenePack: one DATA-AMB file as loaded in memory
// -----------------------------------------------------------------------------
class ScenePack {
public:
    explicit ScenePack(const std::filesystem::path& path);

    // No functions implemented; you can add parse/load later.
private:
    std::filesystem::path m_path;

    DataAmbHeader m_header{};
    std::vector<TocEntry> m_toc;

    // Raw chunks (optional to keep; you can discard after building runtime):
    std::vector<ImageChunk>    m_image_chunks;
    std::vector<AtlasChunk>    m_atlas_chunks;
    std::vector<MapLayerChunk> m_layer_chunks;
    std::vector<EntitiesChunk> m_entities_chunks;
    std::vector<AudioChunk>    m_audio_chunks;
    std::vector<StringsChunk>  m_strings_chunks;

    // Runtime objects (what your engine uses):
    std::vector<RuntimeImage>    m_images;
    std::vector<RuntimeAtlas>    m_atlases;
    std::vector<RuntimeLayer>    m_layers;
    std::vector<RuntimeEntities> m_entities;
    std::vector<RuntimeAudio>    m_audio;
    RuntimeStrings               m_strings;  // usually one per pack; optional
};


// -----------------------------------------------------------------------------
// DataMap: higher-level view for “tilemap layers + atlases + texture(s)”
//   This mirrors your earlier class name but aligns with the new container.
// -----------------------------------------------------------------------------
class DataMap {
public:
    explicit DataMap(std::shared_ptr<ScenePack> pack);

    // No functions implemented.
private:
    std::shared_ptr<ScenePack> m_pack;

    // Which layers/atlases/images are “active” could be stored here later.
};

#endif // DATAAMB_FORMAT_HXX_INCLUDED
