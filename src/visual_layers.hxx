#ifndef VISUAL_LAYERS_HXX_INCLUDED
#define VISUAL_LAYERS_HXX_INCLUDED

#include "runtime_image.hxx"
#include "runtime_atlas.hxx"
#include "runtime_map.hxx"
#include "config.hxx"

#include <memory>
#include <utility>

class VisualLayer {
public:
    VisualLayer(ImageRuntime image_runtime, AtlasRuntime atlas_runtime)
    : m_image_runtime(std::move(image_runtime)),
      m_atlas_runtime(std::move(atlas_runtime)) {}

    virtual ~VisualLayer() = default;

    virtual void render(SDL_Renderer* renderer) = 0;

    ImageRuntime& image() noexcept { return m_image_runtime; }
    const ImageRuntime& image() const noexcept { return m_image_runtime; }

    AtlasRuntime& atlas() noexcept { return m_atlas_runtime; }
    const AtlasRuntime& atlas() const noexcept { return m_atlas_runtime; }

private:
    ImageRuntime m_image_runtime;
    AtlasRuntime m_atlas_runtime;
};

using VisualLayerPtr = std::unique_ptr<VisualLayer>;

class MapLayer final : public VisualLayer {
public:
    MapLayer(ImageRuntime image_runtime,
             AtlasRuntime atlas_runtime,
             MapRuntime map_runtime,
             amb::runtime::SpawnPoint spawn_point)
    : VisualLayer(std::move(image_runtime), std::move(atlas_runtime)),
      m_map_runtime(std::move(map_runtime)),
      m_spawn_point(std::move(spawn_point)) {}

    void render(SDL_Renderer* renderer) override {
        if (renderer == nullptr || image().texture == nullptr || amb::game::MAP_TILE_SIZE == 0) {
            return;
        }

        SDL_Rect viewport {0, 0, 0, 0};
        if (!SDL_GetRenderViewport(renderer, &viewport)) {
            SDL_Log("MapLayer::render failed to query viewport: %s", SDL_GetError());
            return;
        }

        i32 min_tx = 0;
        i32 max_tx = -1;
        i32 min_ty = 0;
        i32 max_ty = -1;
        map().clampVisibleWorldToTileRange(
            0.0f,
            0.0f,
            static_cast<float>(viewport.w),
            static_cast<float>(viewport.h),
            min_tx,
            max_tx,
            min_ty,
            max_ty);

        if (max_tx < min_tx || max_ty < min_ty) {
            return;
        }

        for (i32 tile_y = min_ty; tile_y <= max_ty; ++tile_y) {
            for (i32 tile_x = min_tx; tile_x <= max_tx; ++tile_x) {
                const Cell* cell = map().cellAtTile(static_cast<std::size_t>(tile_x), static_cast<std::size_t>(tile_y));
                if (cell == nullptr) {
                    continue;
                }

                const std::size_t atlas_index = static_cast<std::size_t>(*cell);
                if (atlas_index >= atlas().rects.size()) {
                    continue;
                }

                const SDL_FRect& source_rect = atlas().rects[atlas_index];
                const SDL_FRect destination_rect {
                    static_cast<float>(tile_x * amb::game::MAP_TILE_SIZE),
                    static_cast<float>(tile_y * amb::game::MAP_TILE_SIZE),
                    static_cast<float>(amb::game::MAP_TILE_SIZE),
                    static_cast<float>(amb::game::MAP_TILE_SIZE),
                };

                SDL_RenderTexture(renderer, image().texture.get(), &source_rect, &destination_rect);
            }
        }
    }

    MapRuntime& map() noexcept { return m_map_runtime; }
    const MapRuntime& map() const noexcept { return m_map_runtime; }

    amb::runtime::SpawnPoint& spawnPoint() noexcept { return m_spawn_point; }
    const amb::runtime::SpawnPoint& spawnPoint() const noexcept { return m_spawn_point; }

private:
    MapRuntime m_map_runtime;
    amb::runtime::SpawnPoint m_spawn_point;
};

class SpriteLayer : public VisualLayer {
public:
    virtual ~SpriteLayer() = default;
};

class EffectLayer : public VisualLayer {
public:
    virtual ~EffectLayer() = default;
};

#endif
