#include "damb_runtime.hxx"
#include <cmath>

inline size_t MapRuntime::indexOf(float world_x, float world_y) const noexcept {
    const size_t tile_x = worldToTileX(world_x);
    if (tile_x == amb::runtime::INDEX_NPOS) {
        return amb::runtime::INDEX_NPOS;
    }

    const size_t tile_y = worldToTileY(world_y);
    if (tile_y == amb::runtime::INDEX_NPOS) {
        return amb::runtime::INDEX_NPOS;
    }

    return indexOfTile(tile_x, tile_y);
}

inline bool MapRuntime::inBounds(float world_x, float world_y) const noexcept {
    if (m_tile_width == 0 || m_tile_height == 0 || world_x < 0.0f || world_y < 0.0f) {
        return false;
    }

    const float max_world_x = static_cast<float>(m_width)  * static_cast<float>(m_tile_width);
    const float max_world_y = static_cast<float>(m_height) * static_cast<float>(m_tile_height);

    return world_x < max_world_x && world_y < max_world_y;
}

inline Cell* MapRuntime::tryCell(float world_x, float world_y) noexcept {
    const size_t idx = indexOf(world_x, world_y);
    return (idx == amb::runtime::INDEX_NPOS || idx >= atlas_idx.size()) ? nullptr : &atlas_idx[idx];
}

inline const Cell* MapRuntime::tryCell(float world_x, float world_y) const noexcept {
    const size_t idx = indexOf(world_x, world_y);
    return (idx == amb::runtime::INDEX_NPOS || idx >= atlas_idx.size()) ? nullptr : &atlas_idx[idx];
}

inline void MapRuntime::clampVisibleWorldToTileRange(
    float world_left, float world_top,
    float world_right, float world_bottom,
    i32& min_tx, i32& max_tx,
    i32& min_ty, i32& max_ty) const noexcept
{
    if (m_width == 0 || m_height == 0 || m_tile_width == 0 || m_tile_height == 0) {
        min_tx = min_ty = 0;
        max_tx = max_ty = -1; // empty
        return;
    }

    // world pixels -> tile coords (inclusive tile span)
    min_tx = static_cast<i32>(std::floor(world_left   / static_cast<float>(m_tile_width)));
    min_ty = static_cast<i32>(std::floor(world_top    / static_cast<float>(m_tile_height)));
    max_tx = static_cast<i32>(std::floor((world_right  - 1.0f) / static_cast<float>(m_tile_width)));
    max_ty = static_cast<i32>(std::floor((world_bottom - 1.0f) / static_cast<float>(m_tile_height)));

    // clamp to map tile bounds
    const i32 max_valid_x = static_cast<i32>(m_width) - 1;
    const i32 max_valid_y = static_cast<i32>(m_height) - 1;

    if (min_tx < 0) min_tx = 0;
    if (min_ty < 0) min_ty = 0;
    if (max_tx > max_valid_x) max_tx = max_valid_x;
    if (max_ty > max_valid_y) max_ty = max_valid_y;

    if (max_tx < min_tx) max_tx = min_tx - 1; // preserve empty
    if (max_ty < min_ty) max_ty = min_ty - 1;
}

inline size_t MapRuntime::worldToTileX(float world_x) const noexcept {
    if (m_tile_width == 0 || world_x < 0.0f) {
        return amb::runtime::INDEX_NPOS;
    }

    // Keep float -> size_t conversion after validity checks and explicit flooring.
    const size_t tx = static_cast<size_t>(
        std::floor(world_x / static_cast<float>(m_tile_width))
    );

    return (tx < m_width) ? tx : amb::runtime::INDEX_NPOS;
}

inline size_t MapRuntime::worldToTileY(float world_y) const noexcept {
    if (m_tile_height == 0 || world_y < 0.0f) {
        return amb::runtime::INDEX_NPOS;
    }

    // Keep float -> size_t conversion after validity checks and explicit flooring.
    const size_t ty = static_cast<size_t>(
        std::floor(world_y / static_cast<float>(m_tile_height))
    );

    return (ty < m_height) ? ty : amb::runtime::INDEX_NPOS;
}

inline size_t MapRuntime::indexOfTile(size_t tile_x, size_t tile_y) const noexcept {
    if (tile_x >= m_width || tile_y >= m_height) {
        return amb::runtime::INDEX_NPOS;
    }

    // Row-major flattening: y * width + x.
    return (tile_y * m_width) + tile_x;
}
