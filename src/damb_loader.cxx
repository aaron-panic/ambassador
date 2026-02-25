#include "damb_loader.hxx"

#include "format_damb.hxx"

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    namespace damb = amb::damb;

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

    std::unordered_map<u16, u32> collectAtlasRecordCountsBeforeMapLayer(
        std::ifstream& stream,
        const damb::Header& header,
        const u64 mapl_offset)
    {
        std::unordered_map<u16, u32> atlas_record_counts;

        stream.seekg(static_cast<std::streamoff>(header.toc_offset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error("Failed to seek to TOC.");
        }

        for (u32 i = 0; i < header.toc_count; i++) {
            const damb::TocEntry entry = readPod<damb::TocEntry>(stream, "TOC entry");

            if (entry.offset >= mapl_offset) {
                continue;
            }

            if (!hasChunkType(entry.type, damb::CL_ATLAS)) {
                continue;
            }

            stream.seekg(static_cast<std::streamoff>(entry.offset), std::ios::beg);
            if (!stream) {
                throw std::runtime_error("Failed to seek to ATLS chunk.");
            }

            const damb::AtlasChunkHeader atlas_header = readPod<damb::AtlasChunkHeader>(stream, "ATLS header");
            if (!hasChunkType(atlas_header.header.type, damb::CL_ATLAS)) {
                throw std::runtime_error("TOC ATLS entry points to a non-ATLS chunk.");
            }

            atlas_record_counts[atlas_header.header.id] = atlas_header.asset_count;

            stream.seekg(
                static_cast<std::streamoff>(header.toc_offset + (static_cast<u64>(i + 1u) * damb::TOC_ENTRY_SIZE)),
                std::ios::beg
            );
            if (!stream) {
                throw std::runtime_error("Failed to seek to next TOC entry.");
            }
        }

        return atlas_record_counts;
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

    const std::unordered_map<u16, u32> atlas_record_counts =
        collectAtlasRecordCountsBeforeMapLayer(stream, header, mapl_entry.offset);

    const auto atlas_it = atlas_record_counts.find(mapl_header.atlas_id);
    if (atlas_it == atlas_record_counts.end()) {
        throw std::runtime_error(
            "Missing atlas dependency for MAPL chunk. Expected atlas_id=" +
            std::to_string(mapl_header.atlas_id) + " to reference an ATLS chunk appearing before MAPL."
        );
    }

    stream.seekg(static_cast<std::streamoff>(mapl_entry.offset + damb::MAPL_HEADER_SIZE), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to MAPL cells.");
    }

    const std::size_t cell_count = checkedCellCount(mapl_header.width, mapl_header.height);

    std::vector<damb::MapCell> cells(cell_count);
    stream.read(reinterpret_cast<char*>(cells.data()), static_cast<std::streamsize>(cells.size() * sizeof(damb::MapCell)));
    if (!stream) {
        throw std::runtime_error("Failed to read MAPL cells.");
    }

    MapRuntime map_runtime(mapl_header.width, mapl_header.height);
    auto& runtime_cells = map_runtime.cells();
    runtime_cells.reserve(cell_count);

    for (const damb::MapCell& cell : cells) {
        if (cell.atlas_record_index >= atlas_it->second) {
            throw std::runtime_error(
                "MAPL cell atlas_record_index out of range for referenced atlas."
            );
        }

        runtime_cells.push_back(cell.atlas_record_index);
    }

    const amb::runtime::SpawnPoint spawn_point = map_runtime.defaultSpawnPoint();

    ImageRuntime image_runtime {};
    AtlasRuntime atlas_runtime {};

    return std::make_unique<MapLayer>(
        std::move(image_runtime),
        std::move(atlas_runtime),
        std::move(map_runtime),
        spawn_point
    );
}
