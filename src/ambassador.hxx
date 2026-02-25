#ifndef AMBASSADOR_HXX_INCLUDED
#define AMBASSADOR_HXX_INCLUDED

#include "amb_types.hxx"
#include "damb_loader.hxx"

#include <SDL3/SDL.h>

#include <filesystem>
#include <vector>

// store current app state and needed pointers
class Ambassador {
public:
    Ambassador();
    ~Ambassador();

    SDL_Window* window() const noexcept { return m_window.get(); }
    SDL_Renderer* renderer() const noexcept { return m_renderer.get(); }

    SDL_AppResult checkInit();

    u64 last() { return m_lasttick; }

    bool needUpdate(u64 now);

    SDL_AppResult event(SDL_Event* event);
    SDL_AppResult loop();
    void update(u64 now);
    SDL_AppResult render();
    SDL_AppResult loadSandbox(const std::filesystem::path& file_path);

    void configureGrid(int width, int height);
private:
    WindowPtr m_window;
    RendererPtr m_renderer;

    bool m_initErrors = false;
    u64 m_lasttick = 0;

    int m_viewport_row_sz;
    int m_viewport_col_sz;

    DambLoader m_loader;
    std::vector<VisualLayerPtr> m_layers;
};



#endif
