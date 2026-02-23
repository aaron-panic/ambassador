#include <SDL3/SDL.h>
#include "ambassador.hxx"

SDL_AppResult Ambassador::loop() {
    uint64_t now = SDL_GetTicks();
    if (needUpdate(now)) update();
    return render();
    m_lasttick = now;
}

void Ambassador::update() {
    // foo
}