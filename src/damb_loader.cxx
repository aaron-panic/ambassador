#include "damb_loader.hxx"

#include "damb_format.hxx"

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    namespace damb = amb::damb;
    constexpr u16 DEFAULT_TILE_SIZE = 1;

    template <typename T>
    T readPod(std::ifstream& stream, const std::string& context) {
        T value {};
        stream.read(reinterpret_cast<char*>(&value), sizeof(T));

        if (!stream) {
            throw std::runtime_error("Failed to read " + context + ".");
        }

        return value;
    }

    std::size_t checkedCellCount(u32 width, u32 height) {
        if (width == 0 || height == 0) {
            throw std::runtime_error("Map dimensions must be greater than zero.");
        }

        const u64 count = static_cast<u64>(width) * static_cast<u64>(height);
        if (count > static_cast<u64>(std::numeric_limits<std::size_t>::max())) {
            throw std::runtime_error("Map is too large for this platform.");
        }

        return static_cast<std::size_t>(count);
    }

    bool hasChunkType(const char (&type)[4], const char* expected) {
        return std::memcmp(type, expected, 4) == 0;
    }

    damb::TocEntry findMapLayerEntry(std::ifstream& stream, const damb::Header& header) {
        stream.seekg(static_cast<std::streamoff>(header.toc_offset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error("Failed to seek to TOC.");
        }

        for (u32 i = 0; i < header.toc_count; i++) {
            const damb::TocEntry entry = readPod<damb::TocEntry>(stream, "TOC entry");

            if (hasChunkType(entry.type, damb::CL_MAP_LAYER)) {
                return entry;
            }
        }

        throw std::runtime_error("No MAPL chunk found in file.");
    }
}

VisualLayerPtr DambLoader::loadMapLayer(const std::filesystem::path& file_path) const {
    std::ifstream stream(file_path, std::ios::binary);
    if (!stream.is_open()) {
        throw std::runtime_error("Unable to open file: " + file_path.string());
    }

    const damb::Header header = readPod<damb::Header>(stream, "file header");

    if (std::memcmp(header.magic, damb::MAGIC, 8) != 0) {
        throw std::runtime_error("Invalid DAMB magic value.");
    }

    if (header.version != damb::VERSION) {
        throw std::runtime_error("Unsupported DAMB version.");
    }

    if (header.toc_entry_size != damb::TOC_ENTRY_SIZE) {
        throw std::runtime_error("Unexpected TOC entry size.");
    }

    const damb::TocEntry mapl_entry = findMapLayerEntry(stream, header);

    stream.seekg(static_cast<std::streamoff>(mapl_entry.offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to MAPL chunk.");
    }

    const damb::MapLayerChunkHeader mapl_header = readPod<damb::MapLayerChunkHeader>(stream, "MAPL header");

    if (!hasChunkType(mapl_header.header.type, damb::CL_MAP_LAYER)) {
        throw std::runtime_error("TOC MAPL entry points to a non-MAPL chunk.");
    }

    if (mapl_header.encoding != damb::MapEncoding::raw) {
        throw std::runtime_error("Only raw map encoding is supported.");
    }

    const std::size_t cell_count = checkedCellCount(mapl_header.width, mapl_header.height);

    std::vector<damb::MapCell> cells(cell_count);
    stream.read(reinterpret_cast<char*>(cells.data()), static_cast<std::streamsize>(cells.size() * sizeof(damb::MapCell)));
    if (!stream) {
        throw std::runtime_error("Failed to read MAPL cells.");
    }

    MapRuntime map_runtime(mapl_header.width, mapl_header.height, DEFAULT_TILE_SIZE, DEFAULT_TILE_SIZE);
    auto& runtime_cells = map_runtime.cells();
    runtime_cells.reserve(cell_count);

    for (const damb::MapCell& cell : cells) {
        runtime_cells.push_back(cell.atlas_record_index);
    }

    ImageRuntime image_runtime {};
    AtlasRuntime atlas_runtime {};

    return std::make_unique<MapLayer>(std::move(image_runtime), std::move(atlas_runtime), std::move(map_runtime));
}
