#ifndef RUNTIME_ATLAS_HXX_INCLUDED
#define RUNTIME_ATLAS_HXX_INCLUDED

#include "runtime_object.hxx"

#include <vector>

#include <SDL3/SDL.h>

class AtlasRuntime final : public RuntimeObject {
public:
    std::vector<SDL_FRect> rects;

    const char* typeName() const noexcept override { return "AtlasRuntime"; }
};

#endif
