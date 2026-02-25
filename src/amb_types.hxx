#ifndef AMB_TYPES_HXX_INCLUDED
#define AMB_TYPES_HXX_INCLUDED

#include <cstdint>
#include <memory>

#include <SDL3/SDL.h>

typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
typedef std::int16_t i16;
typedef std::int32_t i32;

struct TextureDeleter {
    void operator()(SDL_Texture* t) { if (t) SDL_DestroyTexture(t); }
};

using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;
using Cell = u16;

#endif
