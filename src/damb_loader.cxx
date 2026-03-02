#include "damb_loader.hxx"

#include "utility_binary.hxx"

#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
    namespace damb = amb::damb;
}

void DambLoader::validateFileHeader(const damb::Header& header, const HeaderChunkCounts& chunk_counts) const {
    if (std::memcmp(header.magic, damb::MAGIC, amb::data::MAGIC_LENGTH) != 0) {
        throw std::runtime_error("Invalid DAMB magic value.");
    }

    if (header.version != damb::VERSION) {
        throw std::runtime_error("Unsupported DAMB version.");
    }

    if (header.toc_entry_size != damb::TOC_ENTRY_SIZE) {
        throw std::runtime_error("Unexpected TOC entry size.");
    }

    if (chunk_counts.mapl_count == 0) {
        throw std::runtime_error("DAMB header indicates zero MAPL chunks.");
    }

    if (chunk_counts.atls_count == 0) {
        throw std::runtime_error("DAMB header indicates zero ATLS chunks.");
    }

    if (chunk_counts.ents_count > 0 && (chunk_counts.mapl_count == 0 || chunk_counts.atls_count == 0)) {
        throw std::runtime_error("ENTS chunks require MAPL and ATLS chunk presence.");
    }
}

std::size_t DambLoader::checkedCellCount(u32 width, u32 height) const {
    if (width == 0 || height == 0) {
        throw std::runtime_error("Map dimensions must be greater than zero.");
    }

    const u64 count = static_cast<u64>(width) * static_cast<u64>(height);
    if (count > static_cast<u64>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("Map is too large for this platform.");
    }

    return static_cast<std::size_t>(count);
}

u64 DambLoader::checkedMapPayloadSize(const std::size_t cell_count) const {
    const u64 payload_size = static_cast<u64>(cell_count) * static_cast<u64>(damb::MAPCELL_SIZE);
    if (payload_size > static_cast<u64>(std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error("MAPL payload is too large for stream I/O on this platform.");
    }

    return payload_size;
}

damb::TocEntry DambLoader::findMapLayerEntry(std::ifstream& stream, const damb::Header& header, const HeaderChunkCounts& chunk_counts) const {
    (void) chunk_counts;
    stream.seekg(static_cast<std::streamoff>(header.toc_offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to TOC.");
    }

    for (u32 i = 0; i < header.toc_count; i++) {
        const damb::TocEntry entry = amb::utility::readPod<damb::TocEntry>(stream, "TOC entry");
        if (amb::utility::chunkTypeEquals(entry.type, damb::CL_MAP_LAYER)) {
            return entry;
        }
    }

    throw std::runtime_error("No MAPL chunk found in file.");
}

damb::TocEntry DambLoader::findAtlasEntryByIdBeforeMapLayer(
    std::ifstream& stream,
    const damb::Header& header,
    const u16 atlas_id,
    const u64 mapl_offset,
    const HeaderChunkCounts& chunk_counts) const
{
    (void) chunk_counts;
    stream.seekg(static_cast<std::streamoff>(header.toc_offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to TOC.");
    }

    for (u32 i = 0; i < header.toc_count; i++) {
        const damb::TocEntry entry = amb::utility::readPod<damb::TocEntry>(stream, "TOC entry");
        if (entry.offset >= mapl_offset) {
            continue;
        }

        if (!amb::utility::chunkTypeEquals(entry.type, damb::CL_ATLAS) || entry.id != atlas_id) {
            continue;
        }

        return entry;
    }

    throw std::runtime_error(
        "Missing atlas dependency for MAPL chunk. Expected atlas_id=" +
        std::to_string(atlas_id) + " to reference an ATLS chunk appearing before MAPL.");
}

damb::TocEntry DambLoader::findImageEntryByIdBeforeMapLayer(
    std::ifstream& stream,
    const damb::Header& header,
    const u16 image_id,
    const u64 mapl_offset,
    const HeaderChunkCounts& chunk_counts) const
{
    (void) chunk_counts;
    stream.seekg(static_cast<std::streamoff>(header.toc_offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to TOC.");
    }

    for (u32 i = 0; i < header.toc_count; i++) {
        const damb::TocEntry entry = amb::utility::readPod<damb::TocEntry>(stream, "TOC entry");
        if (entry.offset >= mapl_offset) {
            continue;
        }

        if (!amb::utility::chunkTypeEquals(entry.type, damb::CL_IMAGE) || entry.id != image_id) {
            continue;
        }

        return entry;
    }

    throw std::runtime_error(
        "Missing image dependency for MAPL chunk. Expected image_id=" +
        std::to_string(image_id) + " to reference an IMAG chunk appearing before MAPL.");
}

VisualLayerPtr DambLoader::loadMapLayer(SDL_Renderer* renderer, const std::filesystem::path& file_path) const {
    std::ifstream stream(file_path, std::ios::binary);
    if (!stream.is_open()) {
        throw std::runtime_error("Unable to open file: " + file_path.string());
    }

    const damb::Header header = amb::utility::readPod<damb::Header>(stream, "file header");
    const HeaderChunkCounts chunk_counts {
        header.imag_count,
        header.atls_count,
        header.mapl_count,
        header.ents_count,
    };

    validateFileHeader(header, chunk_counts);

    const damb::TocEntry map_entry = findMapLayerEntry(stream, header, chunk_counts);
    const damb::MapLayerChunkHeader map_header = loadMapLayerHeader(stream, map_entry);

    const damb::TocEntry atlas_entry = findAtlasEntryByIdBeforeMapLayer(
        stream,
        header,
        map_header.atlas_id,
        map_entry.offset,
        chunk_counts);
    const AtlasChunkRuntimeData atlas_runtime_data = loadAtlasRuntime(stream, atlas_entry);

    const damb::TocEntry image_entry = findImageEntryByIdBeforeMapLayer(
        stream,
        header,
        atlas_runtime_data.metadata.image_id,
        map_entry.offset,
        chunk_counts);
    ImageRuntime image_runtime = loadImageRuntime(stream, image_entry, renderer);

    MapRuntime map_runtime = loadMapRuntime(stream, map_entry, map_header, atlas_runtime_data.metadata);
    const amb::runtime::SpawnPoint spawn_point = map_runtime.defaultSpawnPoint();

    return std::make_unique<MapLayer>(
        std::move(image_runtime),
        std::move(atlas_runtime_data.atlas_runtime),
        std::move(map_runtime),
        spawn_point);
}
