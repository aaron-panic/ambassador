#ifndef DAMB_RUNTIME_HXX_INCLUDED
#define DAMB_RUNTIME_HXX_INCLUDED

#include "amb_types.hxx"
#include <cstddef>
#include <vector>
#include <memory>
#include <limits>
#include <SDL3/SDL.h>

namespace amb::runtime {
    const std::size_t INDEX_NPOS = std::numeric_limits<std::size_t>::max();
}

struct TextureDeleter {
    void operator()(SDL_Texture* t) { if (t) SDL_DestroyTexture(t); }
};
using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;
using Cell = u16;

struct ImageRuntime {
    TexturePtr texture;
};

struct AtlasRuntime {
    std::vector<SDL_FRect> rects;
};

class MapRuntime {
public:
    MapRuntime(size_t width, size_t height, u16 tile_width, u16 tile_height)
    : m_width(width), m_height(height), m_tile_width(tile_width), m_tile_height(tile_height) {}

    inline bool validCellCount() const noexcept { return m_width * m_height == atlas_idx.size(); }
    inline size_t indexOf(float world_x, float world_y) const noexcept;
    inline bool inBounds(float world_x, float world_y) const noexcept;
    inline Cell* tryCell(float world_x, float world_y) noexcept;
    inline const Cell* tryCell(float world_x, float world_y) const noexcept;

    inline void clampVisibleWorldToTileRange(
        float world_left, float world_top,
        float world_right, float world_bottom,
        i32& min_tx, i32& max_tx,
        i32& min_ty, i32& max_ty
    ) const noexcept;

    inline size_t worldToTileX(float world_x) const noexcept;
    inline size_t worldToTileY(float world_y) const noexcept;
    inline size_t indexOfTile(size_t tile_x, size_t tile_y) const noexcept;

    inline std::vector<Cell>& cells() noexcept { return atlas_idx; }
    inline const std::vector<Cell>& cells() const noexcept { return atlas_idx; }

private:    
    size_t m_width;
    size_t m_height;
    u16 m_tile_width;
    u16 m_tile_height;
    std::vector<Cell> atlas_idx;
};



#endif
