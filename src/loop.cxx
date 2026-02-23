#include <SDL3/SDL.h>
#include "ambassador.hxx"

SDL_AppResult Ambassador::loop() {
    uint64_t now = SDL_GetTicks();
    if (needUpdate(now)) update(now);
    return render();
}

void Ambassador::update(uint64_t now) {
    m_lasttick = now;
}