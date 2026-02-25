#ifndef RUNTIME_DAMB_HXX_INCLUDED
#define RUNTIME_DAMB_HXX_INCLUDED

#include "amb_types.hxx"
#include "runtime_map.hxx"

#include <vector>

struct ImageRuntime {
    TexturePtr texture;
};

struct AtlasRuntime {
    std::vector<SDL_FRect> rects;
};

#endif
