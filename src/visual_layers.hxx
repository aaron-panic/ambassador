#ifndef VISUAL_LAYERS_HXX_INCLUDED
#define VISUAL_LAYERS_HXX_INCLUDED

#include "runtime_image.hxx"
#include "runtime_atlas.hxx"
#include "runtime_map.hxx"

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
        (void)renderer;
        // TODO: iterate visible map cells and draw atlas rects from image().texture.
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
