#include "damb_loader.hxx"

#include "format_damb.hxx"

#include <SDL3_image/SDL_image.h>

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    namespace damb = amb::damb;

    struct AtlasChunkMetadata {
        u32 asset_count = 0;
        u16 image_id = 0;
    };

    struct AtlasChunkRuntimeData {
        AtlasRuntime atlas_runtime {};
        AtlasChunkMetadata metadata {};
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

    u64 checkedMapPayloadSize(const std::size_t cell_count) {
        const u64 payload_size = static_cast<u64>(cell_count) * static_cast<u64>(damb::MAPCELL_SIZE);
        if (payload_size > static_cast<u64>(std::numeric_limits<std::streamsize>::max())) {
            throw std::runtime_error("MAPL payload is too large for stream I/O on this platform.");
        }

        return payload_size;
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

    damb::TocEntry findAtlasEntryByIdBeforeMapLayer(
        std::ifstream& stream,
        const damb::Header& header,
        const u16 atlas_id,
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

            if (!hasChunkType(entry.type, damb::CL_ATLAS) || entry.id != atlas_id) {
                continue;
            }

            return entry;
        }

        throw std::runtime_error(
            "Missing atlas dependency for MAPL chunk. Expected atlas_id=" +
            std::to_string(atlas_id) + " to reference an ATLS chunk appearing before MAPL."
        );
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

    AtlasChunkRuntimeData loadAtlasRuntimeFromChunk(std::ifstream& stream, const damb::TocEntry& atlas_entry) {
        stream.seekg(static_cast<std::streamoff>(atlas_entry.offset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error("Failed to seek to ATLS chunk.");
        }

        const damb::AtlasChunkHeader atlas_header = readPod<damb::AtlasChunkHeader>(stream, "ATLS header");
        if (!hasChunkType(atlas_header.header.type, damb::CL_ATLAS)) {
            throw std::runtime_error("TOC ATLS entry points to a non-ATLS chunk.");
        }

        if (atlas_header.header.id != atlas_entry.id) {
            throw std::runtime_error("TOC ATLS entry id does not match ATLS chunk header id.");
        }

        if (atlas_entry.size < damb::ATLS_HEADER_SIZE) {
            throw std::runtime_error("ATLS TOC size is smaller than ATLS header size.");
        }

        const u64 record_bytes = atlas_entry.size - static_cast<u64>(damb::ATLS_HEADER_SIZE);
        if ((record_bytes % static_cast<u64>(damb::ATLS_RECORD_SIZE)) != 0) {
            throw std::runtime_error("ATLS payload size is not aligned to AtlasRecord size.");
        }

        const u64 toc_record_count = record_bytes / static_cast<u64>(damb::ATLS_RECORD_SIZE);
        if (toc_record_count != static_cast<u64>(atlas_header.asset_count)) {
            throw std::runtime_error("ATLS record count does not match ATLS header asset_count.");
        }

        if (toc_record_count > static_cast<u64>(std::numeric_limits<std::size_t>::max())) {
            throw std::runtime_error("ATLS has too many records for this platform.");
        }

        std::vector<damb::AtlasRecord> records(static_cast<std::size_t>(toc_record_count));
        if (!records.empty()) {
            stream.read(reinterpret_cast<char*>(records.data()), static_cast<std::streamsize>(records.size() * sizeof(damb::AtlasRecord)));
            if (!stream) {
                throw std::runtime_error("Failed to read ATLS records.");
            }
        }

        AtlasRuntime atlas_runtime {};
        atlas_runtime.rects.reserve(records.size());

        for (const damb::AtlasRecord& record : records) {
            atlas_runtime.rects.push_back(SDL_FRect {
                static_cast<float>(record.src_x),
                static_cast<float>(record.src_y),
                static_cast<float>(record.src_w),
                static_cast<float>(record.src_h),
            });
        }

        return AtlasChunkRuntimeData {
            std::move(atlas_runtime),
            AtlasChunkMetadata {
                atlas_header.asset_count,
                atlas_header.image_id,
            }
        };
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

    if (mapl_header.header.id != mapl_entry.id) {
        throw std::runtime_error("TOC MAPL entry id does not match MAPL chunk header id.");
    }

    if (mapl_header.encoding != damb::MapEncoding::raw) {
        throw std::runtime_error("Only raw map encoding is supported.");
    }

    if (mapl_entry.size < damb::MAPL_HEADER_SIZE) {
        throw std::runtime_error("MAPL TOC size is smaller than MAPL header size.");
    }

    const std::size_t cell_count = checkedCellCount(mapl_header.width, mapl_header.height);
    const u64 expected_payload_size = checkedMapPayloadSize(cell_count);
    const u64 map_payload_size = mapl_entry.size - static_cast<u64>(damb::MAPL_HEADER_SIZE);
    if (map_payload_size != expected_payload_size) {
        throw std::runtime_error("MAPL payload size does not match width/height cell count.");
    }

    const damb::TocEntry atlas_entry = findAtlasEntryByIdBeforeMapLayer(
        stream,
        header,
        mapl_header.atlas_id,
        mapl_entry.offset
    );
    const AtlasChunkRuntimeData atlas_runtime_data = loadAtlasRuntimeFromChunk(stream, atlas_entry);

    const damb::TocEntry imag_entry = findImageEntryByIdBeforeMapLayer(
        stream,
        header,
        atlas_runtime_data.metadata.image_id,
        mapl_entry.offset
    );
    ImageRuntime image_runtime = loadImageRuntimeFromChunk(stream, imag_entry, renderer);

    stream.seekg(static_cast<std::streamoff>(mapl_entry.offset + damb::MAPL_HEADER_SIZE), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to MAPL cells.");
    }

    MapRuntime map_runtime(mapl_header.width, mapl_header.height);
    auto& runtime_cells = map_runtime.cells();
    runtime_cells.reserve(cell_count);

    for (std::size_t i = 0; i < cell_count; ++i) {
        const damb::MapCell cell = readPod<damb::MapCell>(stream, "MAPL cell");

        if (cell.atlas_record_index >= atlas_runtime_data.metadata.asset_count) {
            throw std::runtime_error(
                "MAPL cell atlas_record_index out of range for referenced atlas."
            );
        }

        runtime_cells.push_back(cell.atlas_record_index);
    }

    const amb::runtime::SpawnPoint spawn_point = map_runtime.defaultSpawnPoint();

    return std::make_unique<MapLayer>(
        std::move(image_runtime),
        std::move(atlas_runtime_data.atlas_runtime),
        std::move(map_runtime),
        spawn_point
    );
}
