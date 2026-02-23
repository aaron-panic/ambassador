#ifndef AMB_CONFIG_HXX_INCLUDED
#define AMB_CONFIG_HXX_INCLUDED

#include "amb_types.hxx"

namespace amb{
// configuration constants
namespace config {
    extern const char* APP_TITLE;
    extern const char* APP_VERSION;
    extern const char* APP_IDENTIFIER;

    extern const int DEFAULT_APP_WIDTH;
    extern const int DEFAULT_APP_HEIGHT;

    extern const u64 GAME_SPEED;
    extern const u64 UPDATE_SPEED;
} // namespace amb::config

namespace game {
    extern const u8 TILE_SIZE;
}

} // namespace amb

#endif