#ifndef DAMB_RUNTIME_HXX_INCLUDED
#define DAMB_RUNTIME_HXX_INCLUDED

#include <SDL3/SDL_render.h>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>

#include "amb_types.hxx"
#include "damb_format.hxx"

namespace amb::damb {
struct TextureDeleter {
    void operator()(SDL_Texture* t) const { if (t) SDL_DestroyTexture(t); }
};
using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;

typedef struct RuntimeImage {
    u32 id = 0;
    u32 width = 0;
    u32 height = 0;

    TexturePtr texture;

} RuntimeImage;

typedef struct RuntimeAtlas {
    u32 id = 0;
    u32 image_id = 0;
    std::vector<SDL_FRect> src_rects;
    std::vector<u32> flags;
} RuntimeAtlas;

typedef struct RuntimeMapLayer {
    u32 id = 0;
    u32 width = 0;
    u32 height = 0;
    i32 z = 0;
    u16 atlas_id = 0;
    std::vector<MapCell> cells;
} RuntimeMapLayer;

}
#endif