#include <SDL3/SDL.h>
#include "ambassador.hxx"

SDL_AppResult Ambassador::loop() {
    if (!m_running) {
        return SDL_APP_CONTINUE;
    }

    u64 now = SDL_GetTicks();
    if (needUpdate(now)) {
        update(now);
    }

    return render();
}

void Ambassador::update(u64 now) {
    m_lasttick = now;
}
