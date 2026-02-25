#include "damb_loader.hxx"

#include "format_damb.hxx"

#include <SDL3_image/SDL_image.h>

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    namespace damb = amb::damb;

    struct AtlasChunkMetadata {
        u32 asset_count = 0;
        u16 image_id = 0;
    };

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

    std::unordered_map<u16, AtlasChunkMetadata> collectAtlasMetadataBeforeMapLayer(
        std::ifstream& stream,
        const damb::Header& header,
        const u64 mapl_offset)
    {
        std::unordered_map<u16, AtlasChunkMetadata> atlas_metadata_by_id;

        stream.seekg(static_cast<std::streamoff>(header.toc_offset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error("Failed to seek to TOC.");
        }

        for (u32 i = 0; i < header.toc_count; i++) {
            const damb::TocEntry entry = readPod<damb::TocEntry>(stream, "TOC entry");

            if (entry.offset >= mapl_offset || !hasChunkType(entry.type, damb::CL_ATLAS)) {
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

            atlas_metadata_by_id[atlas_header.header.id] = AtlasChunkMetadata {
                atlas_header.asset_count,
                atlas_header.image_id,
            };

            stream.seekg(
                static_cast<std::streamoff>(header.toc_offset + (static_cast<u64>(i + 1u) * damb::TOC_ENTRY_SIZE)),
                std::ios::beg
            );
            if (!stream) {
                throw std::runtime_error("Failed to seek to next TOC entry.");
            }
        }

        return atlas_metadata_by_id;
    }

    damb::TocEntry findImageEntryByIdBeforeMapLayer(
        std::ifstream& stream,
        const damb::Header& header,
        const u16 image_id,
        const u64 mapl_offset)
    {
        stream.seekg(static_cast<std::streamoff>(header.toc_offset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error("Failed to seek to TOC.");
        }

        for (u32 i = 0; i < header.toc_count; i++) {
            const damb::TocEntry entry = readPod<damb::TocEntry>(stream, "TOC entry");

            if (entry.offset >= mapl_offset) {
                continue;
            }

            if (!hasChunkType(entry.type, damb::CL_IMAGE) || entry.id != image_id) {
                continue;
            }

            return entry;
        }

        throw std::runtime_error(
            "Missing image dependency for MAPL chunk. Expected image_id=" +
            std::to_string(image_id) + " to reference an IMAG chunk appearing before MAPL."
        );
    }

    ImageRuntime loadImageRuntimeFromChunk(std::ifstream& stream, const damb::TocEntry& imag_entry, SDL_Renderer* renderer) {
        if (renderer == nullptr) {
            throw std::runtime_error("Cannot load IMAG chunk without a valid SDL_Renderer.");
        }

        stream.seekg(static_cast<std::streamoff>(imag_entry.offset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error("Failed to seek to IMAG chunk.");
        }

        const damb::ImageChunkHeader imag_header = readPod<damb::ImageChunkHeader>(stream, "IMAG header");
        if (!hasChunkType(imag_header.header.type, damb::CL_IMAGE)) {
            throw std::runtime_error("TOC IMAG entry points to a non-IMAG chunk.");
        }

        if (imag_header.header.id != imag_entry.id) {
            throw std::runtime_error("TOC IMAG entry id does not match IMAG chunk header id.");
        }

        if (imag_header.format != damb::ImageFormat::png) {
            throw std::runtime_error("Only PNG IMAG chunk format is supported.");
        }

        if (imag_header.size == 0) {
            throw std::runtime_error("IMAG chunk has empty image payload.");
        }

        if (imag_header.size > static_cast<u64>(std::numeric_limits<std::size_t>::max())) {
            throw std::runtime_error("IMAG payload is too large for this platform.");
        }

        const u64 expected_chunk_size = static_cast<u64>(damb::IMAG_HEADER_SIZE) + imag_header.size;
        if (imag_entry.size < expected_chunk_size) {
            throw std::runtime_error("IMAG TOC size is smaller than declared IMAG payload.");
        }

        std::vector<u8> image_blob(static_cast<std::size_t>(imag_header.size));

        stream.read(reinterpret_cast<char*>(image_blob.data()), static_cast<std::streamsize>(image_blob.size()));
        if (!stream) {
            throw std::runtime_error("Failed to read IMAG payload.");
        }

        SDL_IOStream* image_io = SDL_IOFromConstMem(image_blob.data(), static_cast<int>(image_blob.size()));
        if (image_io == nullptr) {
            throw std::runtime_error(std::string("Failed to open IMAG payload as SDL IO stream: ") + SDL_GetError());
        }

        SDL_Texture* raw_texture = IMG_LoadTexture_IO(renderer, image_io, true);
        if (raw_texture == nullptr) {
            throw std::runtime_error(std::string("Failed to decode IMAG payload into texture: ") + SDL_GetError());
        }

        ImageRuntime image_runtime {};
        image_runtime.texture.reset(raw_texture);
        return image_runtime;
    }
}

VisualLayerPtr DambLoader::loadMapLayer(SDL_Renderer* renderer, const std::filesystem::path& file_path) const {
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

    const std::unordered_map<u16, AtlasChunkMetadata> atlas_metadata_by_id =
        collectAtlasMetadataBeforeMapLayer(stream, header, mapl_entry.offset);

    const auto atlas_it = atlas_metadata_by_id.find(mapl_header.atlas_id);
    if (atlas_it == atlas_metadata_by_id.end()) {
        throw std::runtime_error(
            "Missing atlas dependency for MAPL chunk. Expected atlas_id=" +
            std::to_string(mapl_header.atlas_id) + " to reference an ATLS chunk appearing before MAPL."
        );
    }

    const damb::TocEntry imag_entry = findImageEntryByIdBeforeMapLayer(
        stream,
        header,
        atlas_it->second.image_id,
        mapl_entry.offset
    );
    ImageRuntime image_runtime = loadImageRuntimeFromChunk(stream, imag_entry, renderer);

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
        if (cell.atlas_record_index >= atlas_it->second.asset_count) {
            throw std::runtime_error(
                "MAPL cell atlas_record_index out of range for referenced atlas."
            );
        }

        runtime_cells.push_back(cell.atlas_record_index);
    }

    const amb::runtime::SpawnPoint spawn_point = map_runtime.defaultSpawnPoint();

    AtlasRuntime atlas_runtime {};

    return std::make_unique<MapLayer>(
        std::move(image_runtime),
        std::move(atlas_runtime),
        std::move(map_runtime),
        spawn_point
    );
}
