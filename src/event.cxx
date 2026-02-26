#include <SDL3/SDL.h>
#include "ambassador.hxx"

SDL_AppResult Ambassador::event(SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
            return SDL_APP_SUCCESS;
        }

        if (event->key.scancode == SDL_SCANCODE_BACKSLASH && !event->key.repeat) {
            m_running = !m_running;
            if (m_running) {
                m_lasttick = SDL_GetTicks();
            }
        }
    }

    return SDL_APP_CONTINUE;
}
