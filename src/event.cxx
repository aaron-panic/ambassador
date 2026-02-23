#include <SDL3/SDL.h>
#include "ambassador.hxx"

SDL_AppResult Ambassador::event(SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    } else {
        return SDL_APP_CONTINUE;
    }
}