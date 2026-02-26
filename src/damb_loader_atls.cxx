#include "damb_loader.hxx"
#include "damb_atls.hxx"

#include "utility_binary.hxx"

#include <limits>
#include <stdexcept>
#include <vector>

namespace {
    namespace damb = amb::damb;
}

DambLoader::AtlasChunkRuntimeData DambLoader::loadAtlasRuntime(std::ifstream& stream, const damb::TocEntry& atlas_entry) const {
    stream.seekg(static_cast<std::streamoff>(atlas_entry.offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to ATLS chunk.");
    }

    const damb::AtlasChunkHeader atlas_header = amb::utility::readPod<damb::AtlasChunkHeader>(stream, "ATLS header");
    if (!amb::utility::chunkTypeEquals(atlas_header.header.type, damb::CL_ATLAS)) {
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
