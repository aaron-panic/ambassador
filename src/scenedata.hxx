#ifndef SCENEDATA_HXX_INCLUDED
#define SCENEDATA_HXX_INCLUDED

#include <cstddef>
#include <unordered_map>

#include "amb_types.hxx"
#include "damb_runtime.hxx"

class SceneData {
public:
    size_t addImage(const amb::RuntimeImage& image);
    size_t addAtlas(const amb::RuntimeAtlas& atlas);
    size_t addMapLayer(const amb::RuntimeMap& map_layer);

    size_t addImageIndex(u16 id, size_t index);
    size_t addAtlasIndex(u16 id, size_t index);
    size_t addMapLayerIndex(u16 id, size_t index);

    void addAtlasRecord(size_t atlas_index, u16 record_id, const SDL_FRect& src_rect, u32 flags);

    [[nodiscard]] const amb::RuntimeImage* findImage(u16 id) const;
    [[nodiscard]] const amb::RuntimeAtlas* findAtlas(u16 id) const;
    [[nodiscard]] const amb::RuntimeMap* findMapLayer(u16 id) const;

    [[nodiscard]] amb::RuntimeImage* findImage(u16 id);
    [[nodiscard]] amb::RuntimeAtlas* findAtlas(u16 id);
    [[nodiscard]] amb::RuntimeMap* findMapLayer(u16 id);

private:
    amb::RuntimeImageVector m_images;
    amb::RuntimeAtlasVector m_atlases;
    amb::RuntimeMapVector m_map_layers;

    std::unordered_map<u16, size_t> m_index_images;
    std::unordered_map<u16, size_t> m_index_atlases;
    std::unordered_map<u16, size_t> m_index_map_layers;
};

#endif
