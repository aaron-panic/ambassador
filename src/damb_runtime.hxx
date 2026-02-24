#ifndef DAMB_RUNTIME_HXX_INCLUDED
#define DAMB_RUNTIME_HXX_INCLUDED

#include <cstddef>
#include <SDL3/SDL_render.h>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>

#include "amb_types.hxx"

namespace amb {
struct TextureDeleter {
    void operator()(SDL_Texture* t) const { if (t) SDL_DestroyTexture(t); }
};
using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;

struct RuntimeImage {
    u16 id = 0;
    u32 width = 0;
    u32 height = 0;

    TexturePtr texture;
};

// Flattened draw indirection: map cell -> render record -> image texture + src rect
struct RuntimeRenderRecord {
    u16 atlas_id = 0;
    u16 atlas_record_id = 0;

    size_t image_index = 0;
    SDL_FRect src_rect{};
    u32 flags = 0;
};

struct RuntimeAtlas {
    u16 id = 0;
    u16 image_id = 0;

    // Local atlas record order (index == atlas_record_index used in map chunks)
    std::vector<u16> record_ids;
    std::vector<u32> render_record_indices;
};

struct RuntimeMapCell {
    u16 id = 0;
    u32 render_record_index = 0;
};

struct RuntimeMap {
    u16 id = 0;
    u16 atlas_id = 0;

    u32 width = 0;
    u32 height = 0;

    std::vector<RuntimeMapCell> cells; // row major, pre-resolved to render records
};

using RuntimeImageVector = std::vector<RuntimeImage>;
using RuntimeRenderRecordVector = std::vector<RuntimeRenderRecord>;
using RuntimeAtlasVector = std::vector<RuntimeAtlas>;
using RuntimeMapVector = std::vector<RuntimeMap>;

}
#endif
