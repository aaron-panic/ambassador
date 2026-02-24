#ifndef DAMB_RUNTIME_HXX_INCLUDED
#define DAMB_RUNTIME_HXX_INCLUDED

#include <SDL3/SDL_render.h>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>

#include "amb_types.hxx"
#include "damb_format.hxx"

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

struct RuntimeAtlas {
    u16 id = 0;
    u16 image_id = 0;

    std::vector<u16> record_ids;
    std::vector<SDL_FRect> src_rects;
    std::vector<u32> flags;
};

struct RuntimeMap {
    u16 id = 0;
    u16 atlas_id = 0;

    u32 width = 0;
    u32 height = 0;
    i32 z = 0;

    std::vector<amb::damb::MapCell> cells; // row major
};

using RuntimeImageVector = std::vector<RuntimeImage>;
using RuntimeAtlasVector = std::vector<RuntimeAtlas>;
using RuntimeMapVector = std::vector<RuntimeMap>;

}
#endif
