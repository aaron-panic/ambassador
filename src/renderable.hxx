#ifndef RENDERABLE_HXX_INCLUDED
#define RENDERABLE_HXX_INCLUDED

#include <SDL3/SDL.h>

class Renderable {
public:
    virtual void render(SDL_Renderer* renderer) = 0;
};

#endif