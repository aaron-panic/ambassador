#include "ambassador.hxx"
#include "config.hxx"

#include <algorithm>
#include <stdexcept>

Ambassador::Ambassador() {
    SDL_SetAppMetadata(
        amb::config::APP_TITLE,
        amb::config::APP_VERSION,
        amb::config::APP_IDENTIFIER
    );

    m_lasttick = SDL_GetTicks();
}

Ambassador::~Ambassador() = default;

SDL_AppResult Ambassador::bootstrap() {
    if (m_bootstrapped) {
        return checkInit();
    }

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
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY,
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
        SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
    )) {
        m_initErrors = true;
        SDL_Log("Logical presentation setup failed: %s", SDL_GetError());
    }

    configureViewportGrid(amb::config::DEFAULT_APP_WIDTH, amb::config::DEFAULT_APP_HEIGHT);

    m_bootstrapped = true;
    return checkInit();
}

SDL_AppResult Ambassador::checkInit() const {
    return (m_initErrors) ? SDL_APP_FAILURE : SDL_APP_CONTINUE;
}

bool Ambassador::needUpdate(u64 now) {
    return (last() + amb::config::UPDATE_SPEED <= now);
}

void Ambassador::configureViewportGrid(int width, int height) {
    m_viewport_row_sz = width / amb::game::MAP_TILE_SIZE + 1;
    m_viewport_col_sz = height / amb::game::MAP_TILE_SIZE + 1;
}

SDL_Rect Ambassador::layerViewportFor(const VisualLayer& layer) const {
    const auto* map_layer = dynamic_cast<const MapLayer*>(&layer);
    if (map_layer == nullptr) {
        return SDL_Rect {
            0,
            0,
            amb::config::DEFAULT_APP_WIDTH,
            amb::config::DEFAULT_APP_HEIGHT,
        };
    }

    const int map_px_w = static_cast<int>(map_layer->map().width() * amb::game::MAP_TILE_SIZE);
    const int map_px_h = static_cast<int>(map_layer->map().height() * amb::game::MAP_TILE_SIZE);

    const int viewport_w = std::min(map_px_w, amb::config::DEFAULT_APP_WIDTH);
    const int viewport_h = std::min(map_px_h, amb::config::DEFAULT_APP_HEIGHT);

    return SDL_Rect {
        (amb::config::DEFAULT_APP_WIDTH - viewport_w) / 2,
        (amb::config::DEFAULT_APP_HEIGHT - viewport_h) / 2,
        viewport_w,
        viewport_h,
    };
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
