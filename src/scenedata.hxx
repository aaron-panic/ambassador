#ifndef SCENEDATA_HXX_INCLUDED
#define SCENEDATA_HXX_INCLUDED

#include <unordered_map>

#include "amb_types.hxx"
#include "damb_runtime.hxx"


class SceneData {
    

private:
    amb::RuntimeImageVector m_images;
    amb::RuntimeAtlasVector m_atlases;
    amb::RuntimeMapVector m_map_cells;

    std::unordered_map<u16, size_t> m_index_images;
    std::unordered_map<u16, size_t> m_index_atlases;
    std::unordered_map<u16, size_t> m_index_map_cells;
};

#endif