#ifndef RUNTIME_MAP_HXX_INCLUDED
#define RUNTIME_MAP_HXX_INCLUDED

#include "amb_types.hxx"
#include "config.hxx"

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace amb::runtime {
    const std::size_t INDEX_NPOS = std::numeric_limits<std::size_t>::max();

    struct SpawnPoint {
        std::size_t tile_x = 0;
        std::size_t tile_y = 0;
        float world_x = 0.0f;
        float world_y = 0.0f;
        bool is_fallback = true;
    };
}

class MapRuntime {
public:
    MapRuntime(size_t width, size_t height)
    : m_width(width), m_height(height) {}

    inline size_t width() const noexcept { return m_width; }
    inline size_t height() const noexcept { return m_height; }

    inline size_t cellCount() const noexcept { return m_cells.size(); }
    inline bool validCellCount() const noexcept { return m_width * m_height == m_cells.size(); }

    inline const std::vector<Cell>& cells() const noexcept { return m_cells; }

    inline void reserveCells(std::size_t count) { m_cells.reserve(count); }

    inline void appendCell(Cell cell) { m_cells.push_back(cell); }

    inline Cell* tryCell(float world_x, float world_y) noexcept {
        const size_t idx = indexOf(world_x, world_y);
        return (idx == amb::runtime::INDEX_NPOS || idx >= m_cells.size()) ? nullptr : &m_cells[idx];
    }

    inline const Cell* tryCell(float world_x, float world_y) const noexcept {
        const size_t idx = indexOf(world_x, world_y);
        return (idx == amb::runtime::INDEX_NPOS || idx >= m_cells.size()) ? nullptr : &m_cells[idx];
    }

    inline const Cell* cellAtTile(size_t tile_x, size_t tile_y) const noexcept {
        const size_t idx = indexOfTile(tile_x, tile_y);
        return (idx == amb::runtime::INDEX_NPOS || idx >= m_cells.size()) ? nullptr : &m_cells[idx];
    }

    inline size_t indexOf(float world_x, float world_y) const noexcept {
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

    inline bool inBounds(float world_x, float world_y) const noexcept {
        if (amb::game::MAP_TILE_SIZE == 0 || world_x < 0.0f || world_y < 0.0f) {
            return false;
        }

        const float max_world_x = static_cast<float>(m_width)  * static_cast<float>(amb::game::MAP_TILE_SIZE);
        const float max_world_y = static_cast<float>(m_height) * static_cast<float>(amb::game::MAP_TILE_SIZE);

        return world_x < max_world_x && world_y < max_world_y;
    }

    inline void clampVisibleWorldToTileRange(
        float world_left, float world_top,
        float world_right, float world_bottom,
        i32& min_tx, i32& max_tx,
        i32& min_ty, i32& max_ty) const noexcept
    {
        if (m_width == 0 || m_height == 0 || amb::game::MAP_TILE_SIZE == 0) {
            min_tx = min_ty = 0;
            max_tx = max_ty = -1; // empty
            return;
        }

        // world pixels -> tile coords (inclusive tile span)
        min_tx = static_cast<i32>(std::floor(world_left   / static_cast<float>(amb::game::MAP_TILE_SIZE)));
        min_ty = static_cast<i32>(std::floor(world_top    / static_cast<float>(amb::game::MAP_TILE_SIZE)));
        max_tx = static_cast<i32>(std::floor((world_right  - 1.0f) / static_cast<float>(amb::game::MAP_TILE_SIZE)));
        max_ty = static_cast<i32>(std::floor((world_bottom - 1.0f) / static_cast<float>(amb::game::MAP_TILE_SIZE)));

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

    inline size_t worldToTileX(float world_x) const noexcept {
        if (amb::game::MAP_TILE_SIZE == 0 || world_x < 0.0f) {
            return amb::runtime::INDEX_NPOS;
        }

        const size_t tx = static_cast<size_t>(
            std::floor(world_x / static_cast<float>(amb::game::MAP_TILE_SIZE))
        );

        return (tx < m_width) ? tx : amb::runtime::INDEX_NPOS;
    }

    inline size_t worldToTileY(float world_y) const noexcept {
        if (amb::game::MAP_TILE_SIZE == 0 || world_y < 0.0f) {
            return amb::runtime::INDEX_NPOS;
        }

        const size_t ty = static_cast<size_t>(
            std::floor(world_y / static_cast<float>(amb::game::MAP_TILE_SIZE))
        );

        return (ty < m_height) ? ty : amb::runtime::INDEX_NPOS;
    }

    inline size_t indexOfTile(size_t tile_x, size_t tile_y) const noexcept {
        if (tile_x >= m_width || tile_y >= m_height) {
            return amb::runtime::INDEX_NPOS;
        }

        return (tile_y * m_width) + tile_x;
    }

    inline amb::runtime::SpawnPoint defaultSpawnPoint() const noexcept {
        amb::runtime::SpawnPoint spawn {};

        if (m_width == 0 || m_height == 0 || amb::game::MAP_TILE_SIZE == 0) {
            return spawn;
        }

        spawn.tile_x = m_width / 2;
        spawn.tile_y = m_height / 2;

        spawn.world_x = (static_cast<float>(spawn.tile_x) + 0.5f) * static_cast<float>(amb::game::MAP_TILE_SIZE);
        spawn.world_y = (static_cast<float>(spawn.tile_y) + 0.5f) * static_cast<float>(amb::game::MAP_TILE_SIZE);
        spawn.is_fallback = true;
        return spawn;
    }

private:
    size_t m_width;
    size_t m_height;
    std::vector<Cell> m_cells;
};

#endif
