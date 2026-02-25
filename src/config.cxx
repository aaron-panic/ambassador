#include "config.hxx"
#include "ambassador.hxx"

// configuration definitions

// meta info
const char* amb::config::APP_TITLE = "Ambassador";
const char* amb::config::APP_VERSION = "0.1";
const char* amb::config::APP_IDENTIFIER = "io.scavengers.ambassador";

// window definition
const int amb::config::DEFAULT_APP_WIDTH = 800;
const int amb::config::DEFAULT_APP_HEIGHT = 600;

// runtime
const u64 amb::config::GAME_SPEED = 60; // (60 updates/sec)
const u64 amb::config::UPDATE_SPEED = 1000 / GAME_SPEED;

// game constants
const u8 amb::game::MAP_TILE_SIZE = 50;

// initalize static members
SDL_Window* Ambassador::g_window = nullptr;
SDL_Renderer* Ambassador::g_renderer = nullptr;