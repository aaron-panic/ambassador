#ifndef AMBASSADOR_HXX_INCLUDED
#define AMBASSADOR_HXX_INCLUDED

#include <cstdint>
// #include <vector>
#include <SDL3/SDL.h>

// store current app state and needed pointers
class Ambassador {
public:
    Ambassador();
    ~Ambassador();

    static SDL_Window *g_window;
    static SDL_Renderer *g_renderer;

    SDL_AppResult checkInit();

    std::uint64_t last() { return m_lasttick; }

    bool needUpdate(uint64_t now);

    SDL_AppResult event(SDL_Event* event);
    SDL_AppResult loop();
    void update();
    SDL_AppResult render();

    void configureGrid(int width, int height);
private:
    bool m_initErrors = false;
    std::uint64_t m_lasttick = 0;

    int m_viewport_row_sz;
    int m_viewport_col_sz;
};



#endif