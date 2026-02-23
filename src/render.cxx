#include <cstdint>
#include <SDL3/SDL.h>
#include "ambassador.hxx"

SDL_AppResult Ambassador::render() {
    if (!SDL_SetRenderDrawColor(
        g_renderer,
        (uint8_t)255,
        (uint8_t)0,
        (uint8_t)0,
        SDL_ALPHA_OPAQUE
    )) {
        SDL_Log("Renderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_RenderClear(Ambassador::g_renderer);
    SDL_RenderPresent(Ambassador::g_renderer);

    // for (const auto& x : m_layers)
    return SDL_APP_CONTINUE;
}