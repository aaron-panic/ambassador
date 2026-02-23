#include "ambassador.hxx"
#include "config.hxx"

#include <iostream>

const uint8_t Ambassador::TILE_SIZE = 50;

Ambassador::Ambassador() {
    SDL_SetAppMetadata(
        amb::config::APP_TITLE,
        amb::config::APP_VERSION,
        amb::config::APP_IDENTIFIER
    );

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        m_initErrors = true;
        SDL_Log("Video Initialization Error: %s", SDL_GetError());
    }

    if (!m_initErrors && !SDL_CreateWindowAndRenderer(
        amb::config::APP_TITLE,
        amb::config::DEFAULT_APP_WIDTH,
        amb::config::DEFAULT_APP_HEIGHT,
        SDL_WINDOW_RESIZABLE,
        &Ambassador::g_window,
        &Ambassador::g_renderer
    )) {
        m_initErrors = true;
        SDL_Log("Window/Renderer Creation Error: %s", SDL_GetError());
    }

    SDL_SetRenderLogicalPresentation(
        g_renderer,
        amb::config::DEFAULT_APP_WIDTH,
        amb::config::DEFAULT_APP_HEIGHT,
        SDL_LOGICAL_PRESENTATION_LETTERBOX
    );

    configureGrid(amb::config::DEFAULT_APP_WIDTH, amb::config::DEFAULT_APP_HEIGHT);
    m_lasttick = SDL_GetTicks();
}

Ambassador::~Ambassador() {
    // foo
}

SDL_AppResult Ambassador::checkInit() {
    return (m_initErrors) ? SDL_APP_FAILURE : SDL_APP_CONTINUE;
}

bool Ambassador::needUpdate(uint64_t now) {
    return (last() + amb::config::UPDATE_SPEED <= now);
}

void Ambassador::configureGrid(int width, int height) {
    std::cout << "Dimensions: " << width << " X " << height << std::endl;
    m_viewport_row_sz = width / TILE_SIZE + 1;
    m_viewport_col_sz = height / TILE_SIZE + 1;

    std::cout << "Grid Dimensions: " << m_viewport_row_sz << " X " << m_viewport_col_sz << std::endl;
}