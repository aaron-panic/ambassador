#include "ambassador.hxx"
#include "config.hxx"

#include <iostream>
#include <stdexcept>

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

    SDL_Window* raw_window = nullptr;
    SDL_Renderer* raw_renderer = nullptr;

    if (!m_initErrors && !SDL_CreateWindowAndRenderer(
        amb::config::APP_TITLE,
        amb::config::DEFAULT_APP_WIDTH,
        amb::config::DEFAULT_APP_HEIGHT,
        SDL_WINDOW_RESIZABLE,
        &raw_window,
        &raw_renderer
    )) {
        m_initErrors = true;
        SDL_Log("Window/Renderer Creation Error: %s", SDL_GetError());
    }

    if (!m_initErrors) {
        m_window.reset(raw_window);
        m_renderer.reset(raw_renderer);
    }

    if (!m_initErrors && !SDL_SetRenderLogicalPresentation(
        renderer(),
        amb::config::DEFAULT_APP_WIDTH,
        amb::config::DEFAULT_APP_HEIGHT,
        SDL_LOGICAL_PRESENTATION_LETTERBOX
    )) {
        m_initErrors = true;
        SDL_Log("Logical presentation setup failed: %s", SDL_GetError());
    }

    configureGrid(amb::config::DEFAULT_APP_WIDTH, amb::config::DEFAULT_APP_HEIGHT);
    m_lasttick = SDL_GetTicks();
}

Ambassador::~Ambassador() = default;

SDL_AppResult Ambassador::checkInit() {
    return (m_initErrors) ? SDL_APP_FAILURE : SDL_APP_CONTINUE;
}

bool Ambassador::needUpdate(u64 now) {
    return (last() + amb::config::UPDATE_SPEED <= now);
}

void Ambassador::configureGrid(int width, int height) {
    std::cout << "Dimensions: " << width << " X " << height << std::endl;
    m_viewport_row_sz = width / amb::game::MAP_TILE_SIZE + 1;
    m_viewport_col_sz = height / amb::game::MAP_TILE_SIZE + 1;

    std::cout << "Grid Dimensions: " << m_viewport_row_sz << " X " << m_viewport_col_sz << std::endl;
}

SDL_AppResult Ambassador::loadSandbox(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        SDL_Log("DAMB file does not exist: %s", file_path.string().c_str());
        return SDL_APP_FAILURE;
    }

    try {
        m_layers.clear();
        m_layers.emplace_back(m_loader.loadMapLayer(renderer(), file_path));
    } catch (const std::exception& ex) {
        SDL_Log("Failed to load DAMB file %s: %s", file_path.string().c_str(), ex.what());
        return SDL_APP_FAILURE;
    }

    SDL_Log("Loaded DAMB sandbox file: %s", file_path.string().c_str());
    return SDL_APP_CONTINUE;
}
