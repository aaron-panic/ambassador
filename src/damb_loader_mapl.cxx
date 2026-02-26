#include "damb_loader.hxx"

#include "utility_binary.hxx"

#include <stdexcept>

namespace {
    namespace damb = amb::damb;
}

damb::MapLayerChunkHeader DambLoader::loadMapLayerHeader(std::ifstream& stream, const damb::TocEntry& map_entry) const {
    stream.seekg(static_cast<std::streamoff>(map_entry.offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to MAPL chunk.");
    }

    const damb::MapLayerChunkHeader map_header = amb::utility::readPod<damb::MapLayerChunkHeader>(stream, "MAPL header");
    if (!amb::utility::chunkTypeEquals(map_header.header.type, damb::CL_MAP_LAYER)) {
        throw std::runtime_error("TOC MAPL entry points to a non-MAPL chunk.");
    }

    if (map_header.header.id != map_entry.id) {
        throw std::runtime_error("TOC MAPL entry id does not match MAPL chunk header id.");
    }

    if (map_header.encoding != damb::MapEncoding::raw) {
        throw std::runtime_error("Only raw map encoding is supported.");
    }

    if (map_entry.size < damb::MAPL_HEADER_SIZE) {
        throw std::runtime_error("MAPL TOC size is smaller than MAPL header size.");
    }

    return map_header;
}

MapRuntime DambLoader::loadMapRuntime(
    std::ifstream& stream,
    const damb::TocEntry& map_entry,
    const damb::MapLayerChunkHeader& map_header,
    const AtlasChunkMetadata& atlas_metadata) const
{
    const std::size_t cell_count = checkedCellCount(map_header.width, map_header.height);
    const u64 expected_payload_size = checkedMapPayloadSize(cell_count);
    const u64 map_payload_size = map_entry.size - static_cast<u64>(damb::MAPL_HEADER_SIZE);
    if (map_payload_size != expected_payload_size) {
        throw std::runtime_error("MAPL payload size does not match width/height cell count.");
    }

    stream.seekg(static_cast<std::streamoff>(map_entry.offset + damb::MAPL_HEADER_SIZE), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to MAPL cells.");
    }

    MapRuntime map_runtime(map_header.width, map_header.height);
    map_runtime.reserveCells(cell_count);

    for (std::size_t i = 0; i < cell_count; ++i) {
        const damb::MapCell cell = amb::utility::readPod<damb::MapCell>(stream, "MAPL cell");

        if (cell.atlas_record_index >= atlas_metadata.asset_count) {
            throw std::runtime_error("MAPL cell atlas_record_index out of range for referenced atlas.");
        }

        map_runtime.appendCell(cell.atlas_record_index);
    }

    return map_runtime;
}
