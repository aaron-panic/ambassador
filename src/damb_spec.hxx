#ifndef DAMB_SPEC_HXX_INCLUDED
#define DAMB_SPEC_HXX_INCLUDED

#include "damb_atls.hxx"
#include "damb_ents.hxx"
#include "damb_imag.hxx"
#include "damb_mapl.hxx"

#include <filesystem>
#include <vector>

namespace amb::damb {
    struct ImageSpec {
        u16 id = 0;
        std::filesystem::path file_path;
        u32 width = 0;
        u32 height = 0;
        ImageFormat format = ImageFormat::png;
    };

    struct AtlasSpec {
        u16 id = 0;
        u16 image_id = 0;
        std::vector<AtlasRecord> records;
    };

    struct MapSpec {
        u16 id = 0;
        u16 atlas_id = 0;
        u32 width = 0;
        u32 height = 0;
        i32 z = 0;
        std::vector<u16> tile_ids;
    };

    struct EntitySpec {
        EntityType entity_type = EntityType::player;
        u16 atlas_id = 0;
        u16 map_id = 0;
        u16 tile_x = 0;
        u16 tile_y = 0;
        u16 flags = 0;
    };

    struct EntityBlockSpec {
        u16 id = 0;
        u16 map_id = 0;
        std::vector<EntitySpec> records;
    };

    struct ManifestSpec {
        std::filesystem::path output_path;
        ImageSpec image;
        AtlasSpec atlas;
        MapSpec map;
        EntityBlockSpec entity;
        bool has_output = false;
        bool has_image = false;
        bool has_atlas = false;
        bool has_map = false;
        bool has_entity = false;
    };
}

#endif
