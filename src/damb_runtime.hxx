#ifndef DAMB_RUNTIME_HXX_INCLUDED
#define DAMB_RUNTIME_HXX_INCLUDED

#include "damb_format.hxx"
#include "amb_types.hxx"
#include <vector>
#include <memory>
#include <SDL3/SDL.h>


struct TextureDeleter {
    void operator()(SDL_Texture* t) { if (t) SDL_DestroyTexture(t); }
};
using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;
using Cell = u16;

struct ImageRuntime {
    TexturePtr texture;
};

struct AtlasRuntime {
    std::vector<SDL_FRect> rects;
};

struct MapRuntime {
    u32 width;
    u32 height;
    u16 tile_width;
    u16 tile_height;
    std::vector<Cell> atlas_idx;
};



#endif