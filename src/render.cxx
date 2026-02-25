#include <SDL3/SDL.h>
#include "ambassador.hxx"

SDL_AppResult Ambassador::render() {
    if (!SDL_SetRenderDrawColor(
        renderer(),
        (u8)255,
        (u8)0,
        (u8)0,
        SDL_ALPHA_OPAQUE
    )) {
        SDL_Log("Renderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_RenderClear(renderer());
    SDL_RenderPresent(renderer());

    // for (const auto& x : m_layers)
    return SDL_APP_CONTINUE;
}