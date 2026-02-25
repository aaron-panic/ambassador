#ifndef DAMB_LOADER_HXX_INCLUDED
#define DAMB_LOADER_HXX_INCLUDED

#include "visual_layers.hxx"

#include <filesystem>

class DambLoader {
public:
    DambLoader() = default;

    VisualLayerPtr loadMapLayer(SDL_Renderer* renderer, const std::filesystem::path& file_path) const;
};

#endif
