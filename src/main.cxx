#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "ambassador.hxx"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    Ambassador *app = new Ambassador();
    *appstate = app;

    if (app->checkInit() != SDL_APP_CONTINUE) {
        return SDL_APP_FAILURE;
    }

    if (argc < 2) {
        SDL_Log("Usage: ambassador <sandbox.damb>");
        return SDL_APP_FAILURE;
    }

    return app->loadSandbox(argv[1]);
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Ambassador *app = (Ambassador *)appstate;
    return app->loop();
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    Ambassador *app = (Ambassador *)appstate;
    return app->event(event);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)result;
    Ambassador *app = (Ambassador *)appstate;
    delete app;
}
