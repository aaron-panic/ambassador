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
    const u64 dt_ms = now - m_lasttick;
    m_lasttick = now;

    for (const auto& entity : m_entities) {
        if (entity != nullptr) {
            entity->integrate(dt_ms);
        }
    }
}
