#include <SDL3/SDL.h>
#include "ambassador.hxx"

SDL_AppResult Ambassador::render() {
    if (!SDL_SetRenderDrawColor(
        renderer(),
        (u8)0,
        (u8)0,
        (u8)0,
        SDL_ALPHA_OPAQUE
    )) {
        SDL_Log("Renderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_RenderClear(renderer())) {
        SDL_Log("Renderer clear failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    for (const auto& layer : m_layers) {
        const SDL_Rect viewport = layerViewportFor(*layer);
        if (!SDL_SetRenderViewport(renderer(), &viewport)) {
            SDL_Log("Renderer viewport setup failed: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        layer->render(renderer());
    }

    SDL_SetRenderViewport(renderer(), nullptr);
    SDL_RenderPresent(renderer());

    return SDL_APP_CONTINUE;
}
