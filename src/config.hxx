#ifndef AMB_CONFIG_HXX_INCLUDED
#define AMB_CONFIG_HXX_INCLUDED

#include <cstdint>

namespace amb{
// configuration constants
namespace config {
    extern const char* APP_TITLE;
    extern const char* APP_VERSION;
    extern const char* APP_IDENTIFIER;

    extern const int DEFAULT_APP_WIDTH;
    extern const int DEFAULT_APP_HEIGHT;

    extern const uint64_t GAME_SPEED;
    extern const uint64_t UPDATE_SPEED;
} // namespace amb::config

} // namespace amb

#endif