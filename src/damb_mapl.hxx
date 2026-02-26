#ifndef DAMB_MAPL_HXX_INCLUDED
#define DAMB_MAPL_HXX_INCLUDED

#include "damb_format.hxx"

#include <type_traits>

namespace amb::damb {
    enum class MapEncoding : u8 {
        raw = 0
    };

    constexpr u16 MAPCELL_SIZE = 4;
    constexpr u16 MAPL_HEADER_SIZE = 28;

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

        i32 z = 0;

        u16 atlas_id = 0;
        MapEncoding encoding = MapEncoding::raw;

        u8 reserved[5] = {};

    };
    static_assert(sizeof(MapLayerChunkHeader) == MAPL_HEADER_SIZE, "MapLayerChunkHeader size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<MapLayerChunkHeader>, "MapLayerChunkHeader must be POD/trivially copyable.");
}

#endif
