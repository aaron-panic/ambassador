#ifndef AMB_CONFIG_HXX_INCLUDED
#define AMB_CONFIG_HXX_INCLUDED

#include "amb_types.hxx"

namespace amb {
namespace config {
    extern const char* APP_TITLE;
    extern const char* APP_VERSION;
    extern const char* APP_IDENTIFIER;

    extern const int DEFAULT_APP_WIDTH;
    extern const int DEFAULT_APP_HEIGHT;

    extern const u64 GAME_SPEED;
    extern const u64 UPDATE_SPEED;
}

namespace game {
    extern const u8 MAP_TILE_SIZE;
}

namespace data {
    extern const u8 CHUNK_TYPE_LENGTH;
    extern const u8 MAGIC_LENGTH;
}
}

#endif
