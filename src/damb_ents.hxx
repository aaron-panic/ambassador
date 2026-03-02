#ifndef DAMB_ENTS_HXX_INCLUDED
#define DAMB_ENTS_HXX_INCLUDED

#include "damb_format.hxx"

#include <type_traits>

namespace amb::damb {
    enum class EntityType : u8 {
        player = 1,
    };

    constexpr u16 ENTS_HEADER_SIZE = 12;
    constexpr u16 ENTS_RECORD_SIZE = 12;

    struct EntityChunkHeader {
        ChunkHeader header;

        u16 map_id = 0;
        u16 entity_count = 0;
        u16 reserved = 0;
    };
    static_assert(sizeof(EntityChunkHeader) == ENTS_HEADER_SIZE, "EntityChunkHeader size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<EntityChunkHeader>, "EntityChunkHeader must be POD/trivially copyable.");

    struct EntityRecord {
        EntityType entity_type = EntityType::player;
        u8 reserved = 0;
        u16 atlas_id = 0;
        u16 map_id = 0;
        u16 tile_x = 0;
        u16 tile_y = 0;
        u16 flags = 0;
    };
    static_assert(sizeof(EntityRecord) == ENTS_RECORD_SIZE, "EntityRecord size does not match stated value.");
    static_assert(std::is_trivially_copyable_v<EntityRecord>, "EntityRecord must be POD/trivially copyable.");
}

#endif
