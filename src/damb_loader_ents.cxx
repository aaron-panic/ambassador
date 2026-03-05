#include "damb_loader.hxx"
#include "damb_ents.hxx"

#include "utility_binary.hxx"

#include <stdexcept>
#include <string>
#include <vector>

namespace {
    namespace damb = amb::damb;
}

void DambLoader::validateEntsChunks(
    std::ifstream& stream,
    const damb::Header& header,
    const damb::TocEntry& mapl_entry,
    const MapRuntime& map_runtime,
    const HeaderChunkCounts& chunk_counts) const
{
    if (chunk_counts.ents_count == 0) {
        return;
    }

    // Collect all TOC entries once to avoid repeated seeks during per-record atlas lookups.
    stream.seekg(static_cast<std::streamoff>(header.toc_offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to TOC for ENTS validation.");
    }

    std::vector<damb::TocEntry> all_toc_entries;
    all_toc_entries.reserve(header.toc_count);
    for (u32 i = 0; i < header.toc_count; i++) {
        all_toc_entries.push_back(amb::utility::readPod<damb::TocEntry>(stream, "TOC entry"));
    }

    for (const damb::TocEntry& ents_entry : all_toc_entries) {
        if (!amb::utility::chunkTypeEquals(ents_entry.type, damb::CL_ENTITY)) {
            continue;
        }

        // Rule 1: ENTS must appear after MAPL in the file.
        if (ents_entry.offset <= mapl_entry.offset) {
            throw std::runtime_error(
                "ENTS chunk (id=" + std::to_string(ents_entry.id) +
                ") must appear after MAPL chunk in the file. ENTS ordering constraint violated.");
        }

        // Read and validate ENTS chunk header.
        stream.seekg(static_cast<std::streamoff>(ents_entry.offset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error(
                "Failed to seek to ENTS chunk (id=" + std::to_string(ents_entry.id) + ").");
        }

        const damb::EntityChunkHeader ents_header =
            amb::utility::readPod<damb::EntityChunkHeader>(stream, "ENTS header");

        if (!amb::utility::chunkTypeEquals(ents_header.header.type, damb::CL_ENTITY)) {
            throw std::runtime_error("TOC ENTS entry points to a non-ENTS chunk.");
        }

        if (ents_header.header.id != ents_entry.id) {
            throw std::runtime_error("TOC ENTS entry id does not match ENTS chunk header id.");
        }

        // Rule 2: ENTS must reference the loaded MAPL.
        if (ents_header.map_id != mapl_entry.id) {
            throw std::runtime_error(
                "ENTS chunk (id=" + std::to_string(ents_entry.id) +
                ") references map_id=" + std::to_string(ents_header.map_id) +
                " but loaded MAPL has id=" + std::to_string(mapl_entry.id) + ".");
        }

        // Validate each entity record. Stream is positioned immediately after the ENTS header.
        for (u16 r = 0; r < ents_header.entity_count; r++) {
            const damb::EntityRecord record =
                amb::utility::readPod<damb::EntityRecord>(stream, "ENTS entity record");

            // Rule 3: Spawn tile must be within map bounds.
            if (static_cast<std::size_t>(record.tile_x) >= map_runtime.width() ||
                static_cast<std::size_t>(record.tile_y) >= map_runtime.height())
            {
                throw std::runtime_error(
                    "ENTS entity record spawn (" +
                    std::to_string(record.tile_x) + ", " + std::to_string(record.tile_y) +
                    ") is out of map bounds (" +
                    std::to_string(map_runtime.width()) + "x" +
                    std::to_string(map_runtime.height()) + ").");
            }

            // Rule 4: Referenced ATLS must appear before the ENTS chunk in the file.
            // No sprite can be drawn for an entity without a loaded atlas.
            bool atlas_found = false;
            for (const damb::TocEntry& toc : all_toc_entries) {
                if (amb::utility::chunkTypeEquals(toc.type, damb::CL_ATLAS) &&
                    toc.id == record.atlas_id &&
                    toc.offset < ents_entry.offset)
                {
                    atlas_found = true;
                    break;
                }
            }

            if (!atlas_found) {
                throw std::runtime_error(
                    "ENTS entity record references atlas_id=" + std::to_string(record.atlas_id) +
                    " but no ATLS chunk with that id appears before ENTS chunk (id=" +
                    std::to_string(ents_entry.id) + "). No sprite source available.");
            }
        }
    }
}
