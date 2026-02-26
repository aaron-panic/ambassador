#ifndef DAMB_LOADER_HXX_INCLUDED
#define DAMB_LOADER_HXX_INCLUDED

#include "damb_mapl.hxx"
#include "damb_format.hxx"
#include "visual_layers.hxx"

#include <filesystem>
#include <fstream>

class DambLoader {
public:
    DambLoader() = default;

    VisualLayerPtr loadMapLayer(SDL_Renderer* renderer, const std::filesystem::path& file_path) const;

private:
    struct AtlasChunkMetadata {
        u32 asset_count = 0;
        u16 image_id = 0;
    };

    struct AtlasChunkRuntimeData {
        AtlasRuntime atlas_runtime {};
        AtlasChunkMetadata metadata {};
    };

    void validateFileHeader(const amb::damb::Header& header) const;

    amb::damb::TocEntry findMapLayerEntry(std::ifstream& stream, const amb::damb::Header& header) const;
    amb::damb::TocEntry findAtlasEntryByIdBeforeMapLayer(
        std::ifstream& stream,
        const amb::damb::Header& header,
        u16 atlas_id,
        u64 mapl_offset) const;
    amb::damb::TocEntry findImageEntryByIdBeforeMapLayer(
        std::ifstream& stream,
        const amb::damb::Header& header,
        u16 image_id,
        u64 mapl_offset) const;

    amb::damb::MapLayerChunkHeader loadMapLayerHeader(std::ifstream& stream, const amb::damb::TocEntry& map_entry) const;
    AtlasChunkRuntimeData loadAtlasRuntime(std::ifstream& stream, const amb::damb::TocEntry& atlas_entry) const;
    ImageRuntime loadImageRuntime(std::ifstream& stream, const amb::damb::TocEntry& image_entry, SDL_Renderer* renderer) const;
    MapRuntime loadMapRuntime(
        std::ifstream& stream,
        const amb::damb::TocEntry& map_entry,
        const amb::damb::MapLayerChunkHeader& map_header,
        const AtlasChunkMetadata& atlas_metadata) const;

    std::size_t checkedCellCount(u32 width, u32 height) const;
    u64 checkedMapPayloadSize(std::size_t cell_count) const;
};

#endif
