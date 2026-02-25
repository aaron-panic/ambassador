#include "format_damb.hxx"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <type_traits>
#include <vector>

namespace {
    namespace damb = amb::damb;

    struct ImageSpec {
        u16 id = 0;
        std::filesystem::path file_path;
        u32 width = 0;
        u32 height = 0;
        damb::ImageFormat format = damb::ImageFormat::png;
    };

    struct AtlasSpec {
        u16 id = 0;
        u16 image_id = 0;
        std::vector<damb::AtlasRecord> records;
    };

    struct MapSpec {
        u16 id = 0;
        u16 atlas_id = 0;
        u32 width = 0;
        u32 height = 0;
        i32 z = 0;
        std::vector<u16> tile_ids;
    };

    struct ManifestSpec {
        std::filesystem::path output_path;
        ImageSpec image;
        AtlasSpec atlas;
        MapSpec map;
        bool has_output = false;
        bool has_image = false;
        bool has_atlas = false;
        bool has_map = false;
    };

    struct ChunkBlob {
        damb::TocEntry toc {};
        std::vector<u8> bytes;
    };

    std::string trim(const std::string& value) {
        const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };

        const auto first = std::find_if_not(value.begin(), value.end(), is_space);
        if (first == value.end()) {
            return "";
        }

        const auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
        return std::string(first, last);
    }

    std::vector<std::string> split(const std::string& value, char delimiter) {
        std::vector<std::string> result;
        std::stringstream stream(value);
        std::string token;

        while (std::getline(stream, token, delimiter)) {
            result.push_back(token);
        }

        return result;
    }

    std::vector<std::string> splitWhitespace(const std::string& value) {
        std::istringstream stream(value);
        std::vector<std::string> result;
        std::string token;

        while (stream >> token) {
            result.push_back(token);
        }

        return result;
    }

    template <typename T>
    void appendPod(std::vector<u8>& out, const T& pod) {
        static_assert(std::is_trivially_copyable_v<T>, "appendPod requires POD types.");
        const auto* begin = reinterpret_cast<const u8*>(&pod);
        out.insert(out.end(), begin, begin + sizeof(T));
    }

    std::pair<std::string, std::string> parseKeyValue(const std::string& token, std::size_t line_number) {
        const std::size_t separator = token.find('=');
        if (separator == std::string::npos || separator == 0 || separator == token.size() - 1) {
            throw std::runtime_error("Line " + std::to_string(line_number) + ": expected key=value token.");
        }

        return {token.substr(0, separator), token.substr(separator + 1)};
    }

    u64 parseUnsigned(const std::string& value, std::size_t line_number, const std::string& field_name) {
        try {
            std::size_t consumed = 0;
            const unsigned long long parsed = std::stoull(value, &consumed, 10);
            if (consumed != value.size()) {
                throw std::runtime_error("invalid trailing characters");
            }

            return static_cast<u64>(parsed);
        } catch (const std::exception&) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": invalid unsigned integer for " + field_name + "."
            );
        }
    }

    i32 parseSigned32(const std::string& value, std::size_t line_number, const std::string& field_name) {
        try {
            std::size_t consumed = 0;
            const long parsed = std::stol(value, &consumed, 10);
            if (consumed != value.size()) {
                throw std::runtime_error("invalid trailing characters");
            }
            if (parsed < static_cast<long>(std::numeric_limits<i32>::min()) ||
                parsed > static_cast<long>(std::numeric_limits<i32>::max())) {
                throw std::runtime_error("range error");
            }
            return static_cast<i32>(parsed);
        } catch (const std::exception&) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": invalid signed integer for " + field_name + "."
            );
        }
    }

    i16 parseSigned16(const std::string& value, std::size_t line_number, const std::string& field_name) {
        const i32 parsed = parseSigned32(value, line_number, field_name);
        if (parsed < std::numeric_limits<i16>::min() || parsed > std::numeric_limits<i16>::max()) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": value out of range for i16 field " + field_name + "."
            );
        }

        return static_cast<i16>(parsed);
    }

    u16 parseUnsigned16(const std::string& value, std::size_t line_number, const std::string& field_name) {
        const u64 parsed = parseUnsigned(value, line_number, field_name);
        if (parsed > std::numeric_limits<u16>::max()) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": value out of range for u16 field " + field_name + "."
            );
        }

        return static_cast<u16>(parsed);
    }

    u32 parseUnsigned32(const std::string& value, std::size_t line_number, const std::string& field_name) {
        const u64 parsed = parseUnsigned(value, line_number, field_name);
        if (parsed > std::numeric_limits<u32>::max()) {
            throw std::runtime_error(
                "Line " + std::to_string(line_number) + ": value out of range for u32 field " + field_name + "."
            );
        }

        return static_cast<u32>(parsed);
    }

    damb::ImageFormat parseImageFormat(const std::string& value, std::size_t line_number) {
        if (value == "png") {
            return damb::ImageFormat::png;
        }

        throw std::runtime_error("Line " + std::to_string(line_number) + ": unsupported image format: " + value);
    }

    ManifestSpec parseManifest(const std::filesystem::path& manifest_path) {
        std::ifstream stream(manifest_path);
        if (!stream.is_open()) {
            throw std::runtime_error("Unable to open manifest file: " + manifest_path.string());
        }

        enum class ParseState {
            top,
            atlas,
            map,
            rows,
        };

        ManifestSpec manifest;
        ParseState state = ParseState::top;
        std::string line;
        std::size_t line_number = 0;

        bool saw_manifest_header = false;

        while (std::getline(stream, line)) {
            line_number++;
            const std::string cleaned = trim(line);

            if (cleaned.empty() || cleaned[0] == ';') {
                continue;
            }

            const std::vector<std::string> tokens = splitWhitespace(cleaned);

            if (!saw_manifest_header) {
                if (tokens.size() != 2 || tokens[0] != "damb_manifest") {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": first non-comment line must be `damb_manifest 1`.");
                }

                const u64 version = parseUnsigned(tokens[1], line_number, "manifest version");
                if (version != 1) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": only manifest version 1 is supported.");
                }

                saw_manifest_header = true;
                continue;
            }

            if (state == ParseState::rows) {
                if (tokens.size() == 1 && tokens[0] == "endrows") {
                    state = ParseState::map;
                    continue;
                }

                const std::vector<std::string> row_tokens = split(cleaned, '|');
                if (row_tokens.size() != manifest.map.width) {
                    throw std::runtime_error(
                        "Line " + std::to_string(line_number) + ": row width mismatch; expected " +
                        std::to_string(manifest.map.width) + " values separated by '|'."
                    );
                }

                for (const std::string& cell_token_raw : row_tokens) {
                    const std::string cell_token = trim(cell_token_raw);
                    const u16 tile_id = parseUnsigned16(cell_token, line_number, "map tile id");
                    manifest.map.tile_ids.push_back(tile_id);
                }

                continue;
            }

            if (tokens[0] == "output") {
                if (state != ParseState::top || tokens.size() != 2) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": output line must be `output <path>` at top scope.");
                }
                manifest.output_path = tokens[1];
                manifest.has_output = true;
                continue;
            }

            if (tokens[0] == "image") {
                if (state != ParseState::top || tokens.size() != 6) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": image line must be `image <id> <path> <width> <height> <format>`.");
                }

                manifest.image.id = parseUnsigned16(tokens[1], line_number, "image id");
                manifest.image.file_path = tokens[2];
                manifest.image.width = parseUnsigned32(tokens[3], line_number, "image width");
                manifest.image.height = parseUnsigned32(tokens[4], line_number, "image height");
                manifest.image.format = parseImageFormat(tokens[5], line_number);
                manifest.has_image = true;
                continue;
            }

            if (tokens[0] == "atlas") {
                if (state != ParseState::top || tokens.size() != 3) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": atlas line must be `atlas <id> image=<image_id>`.");
                }

                manifest.atlas.id = parseUnsigned16(tokens[1], line_number, "atlas id");
                const auto [key, value] = parseKeyValue(tokens[2], line_number);
                if (key != "image") {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": atlas requires image=<image_id>.");
                }
                manifest.atlas.image_id = parseUnsigned16(value, line_number, "atlas image_id");
                manifest.atlas.records.clear();
                manifest.has_atlas = true;
                state = ParseState::atlas;
                continue;
            }

            if (tokens[0] == "tile") {
                if (state != ParseState::atlas) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": tile entry is only valid inside atlas block.");
                }
                if (tokens.size() < 3) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": tile entry must include id and rect.");
                }

                damb::AtlasRecord record {};
                record.id = parseUnsigned16(tokens[1], line_number, "tile id");
                bool has_rect = false;

                for (std::size_t i = 2; i < tokens.size(); i++) {
                    const auto [key, value] = parseKeyValue(tokens[i], line_number);

                    if (key == "rect") {
                        const std::vector<std::string> values = split(value, ',');
                        if (values.size() != 4) {
                            throw std::runtime_error("Line " + std::to_string(line_number) + ": rect requires x,y,w,h.");
                        }
                        record.src_x = parseUnsigned16(trim(values[0]), line_number, "tile rect x");
                        record.src_y = parseUnsigned16(trim(values[1]), line_number, "tile rect y");
                        record.src_w = parseUnsigned16(trim(values[2]), line_number, "tile rect w");
                        record.src_h = parseUnsigned16(trim(values[3]), line_number, "tile rect h");
                        has_rect = true;
                    } else if (key == "flags") {
                        record.flags = parseUnsigned32(value, line_number, "tile flags");
                    } else if (key == "anchor") {
                        const std::vector<std::string> values = split(value, ',');
                        if (values.size() != 2) {
                            throw std::runtime_error("Line " + std::to_string(line_number) + ": anchor requires x,y.");
                        }
                        record.anchor_x = parseSigned16(trim(values[0]), line_number, "tile anchor x");
                        record.anchor_y = parseSigned16(trim(values[1]), line_number, "tile anchor y");
                    }
                }

                if (!has_rect) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": tile entry is missing rect=x,y,w,h.");
                }

                manifest.atlas.records.push_back(record);
                continue;
            }

            if (tokens[0] == "endatlas") {
                if (state != ParseState::atlas || tokens.size() != 1) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": unexpected endatlas.");
                }
                state = ParseState::top;
                continue;
            }

            if (tokens[0] == "map") {
                if (state != ParseState::top || tokens.size() != 6) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": map line must be `map <id> atlas=<id> width=<w> height=<h> z=<z>`.");
                }

                manifest.map = MapSpec {};
                manifest.map.id = parseUnsigned16(tokens[1], line_number, "map id");

                for (std::size_t i = 2; i < tokens.size(); i++) {
                    const auto [key, value] = parseKeyValue(tokens[i], line_number);
                    if (key == "atlas") {
                        manifest.map.atlas_id = parseUnsigned16(value, line_number, "map atlas_id");
                    } else if (key == "width") {
                        manifest.map.width = parseUnsigned32(value, line_number, "map width");
                    } else if (key == "height") {
                        manifest.map.height = parseUnsigned32(value, line_number, "map height");
                    } else if (key == "z") {
                        manifest.map.z = parseSigned32(value, line_number, "map z");
                    } else {
                        throw std::runtime_error("Line " + std::to_string(line_number) + ": unknown map field: " + key);
                    }
                }

                manifest.has_map = true;
                state = ParseState::map;
                continue;
            }

            if (tokens[0] == "rows") {
                if (state != ParseState::map || tokens.size() != 1) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": rows block must be inside map.");
                }
                state = ParseState::rows;
                continue;
            }

            if (tokens[0] == "endmap") {
                if (state != ParseState::map || tokens.size() != 1) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": unexpected endmap.");
                }
                state = ParseState::top;
                continue;
            }

            throw std::runtime_error("Line " + std::to_string(line_number) + ": unknown statement: " + tokens[0]);
        }

        if (!saw_manifest_header) {
            throw std::runtime_error("Manifest is empty or missing `damb_manifest 1` header.");
        }
        if (state != ParseState::top) {
            throw std::runtime_error("Manifest ended before closing all blocks.");
        }
        if (!manifest.has_output || !manifest.has_image || !manifest.has_atlas || !manifest.has_map) {
            throw std::runtime_error("Manifest must define output, image, atlas, and map blocks.");
        }
        if (manifest.atlas.image_id != manifest.image.id) {
            throw std::runtime_error("Atlas image dependency does not match declared image id.");
        }
        if (manifest.map.atlas_id != manifest.atlas.id) {
            throw std::runtime_error("Map atlas dependency does not match declared atlas id.");
        }
        if (manifest.map.width == 0 || manifest.map.height == 0) {
            throw std::runtime_error("Map width and height must be greater than zero.");
        }

        const u64 expected_cells = static_cast<u64>(manifest.map.width) * static_cast<u64>(manifest.map.height);
        if (manifest.map.tile_ids.size() != expected_cells) {
            throw std::runtime_error(
                "Map row count mismatch; expected " + std::to_string(expected_cells) +
                " cells but parsed " + std::to_string(manifest.map.tile_ids.size()) + "."
            );
        }
        if (manifest.atlas.records.empty()) {
            throw std::runtime_error("Atlas must define at least one tile record.");
        }

        for (const u16 tile_index : manifest.map.tile_ids) {
            if (tile_index >= manifest.atlas.records.size()) {
                throw std::runtime_error(
                    "Map tile index " + std::to_string(tile_index) + " is out of range for atlas record count " +
                    std::to_string(manifest.atlas.records.size()) + "."
                );
            }
        }

        return manifest;
    }

    std::vector<u8> readFileBytes(const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open()) {
            throw std::runtime_error("Unable to open file: " + path.string());
        }

        stream.seekg(0, std::ios::end);
        const std::streamoff size = stream.tellg();
        if (size < 0) {
            throw std::runtime_error("Unable to get file size for: " + path.string());
        }
        stream.seekg(0, std::ios::beg);

        std::vector<u8> bytes(static_cast<std::size_t>(size));
        stream.read(reinterpret_cast<char*>(bytes.data()), size);
        if (!stream) {
            throw std::runtime_error("Failed to read file bytes: " + path.string());
        }

        return bytes;
    }

    ChunkBlob buildImageChunk(const ManifestSpec& manifest, const std::filesystem::path& base_dir) {
        ChunkBlob chunk;

        const std::filesystem::path image_path = base_dir / manifest.image.file_path;
        const std::vector<u8> image_bytes = readFileBytes(image_path);

        damb::ImageChunkHeader header {};
        std::memcpy(header.header.type, damb::CL_IMAGE, 4);
        header.header.id = manifest.image.id;
        header.size = static_cast<u64>(image_bytes.size());
        header.width = manifest.image.width;
        header.height = manifest.image.height;
        header.format = manifest.image.format;

        appendPod(chunk.bytes, header);
        chunk.bytes.insert(chunk.bytes.end(), image_bytes.begin(), image_bytes.end());

        std::memcpy(chunk.toc.type, damb::CL_IMAGE, 4);
        chunk.toc.id = manifest.image.id;
        chunk.toc.size = chunk.bytes.size();
        chunk.toc.uncompressed_size = chunk.toc.size;
        return chunk;
    }

    ChunkBlob buildAtlasChunk(const ManifestSpec& manifest) {
        ChunkBlob chunk;

        damb::AtlasChunkHeader header {};
        std::memcpy(header.header.type, damb::CL_ATLAS, 4);
        header.header.id = manifest.atlas.id;
        header.asset_count = static_cast<u32>(manifest.atlas.records.size());
        header.image_id = manifest.atlas.image_id;

        appendPod(chunk.bytes, header);
        for (const damb::AtlasRecord& record : manifest.atlas.records) {
            appendPod(chunk.bytes, record);
        }

        std::memcpy(chunk.toc.type, damb::CL_ATLAS, 4);
        chunk.toc.id = manifest.atlas.id;
        chunk.toc.size = chunk.bytes.size();
        chunk.toc.uncompressed_size = chunk.toc.size;
        return chunk;
    }

    ChunkBlob buildMapChunk(const ManifestSpec& manifest) {
        ChunkBlob chunk;

        damb::MapLayerChunkHeader header {};
        std::memcpy(header.header.type, damb::CL_MAP_LAYER, 4);
        header.header.id = manifest.map.id;
        header.width = manifest.map.width;
        header.height = manifest.map.height;
        header.z = manifest.map.z;
        header.atlas_id = manifest.map.atlas_id;
        header.encoding = damb::MapEncoding::raw;

        appendPod(chunk.bytes, header);

        for (const u16 atlas_record_index : manifest.map.tile_ids) {
            damb::MapCell cell {};
            cell.id = 0;
            cell.atlas_record_index = atlas_record_index;
            appendPod(chunk.bytes, cell);
        }

        std::memcpy(chunk.toc.type, damb::CL_MAP_LAYER, 4);
        chunk.toc.id = manifest.map.id;
        chunk.toc.size = chunk.bytes.size();
        chunk.toc.uncompressed_size = chunk.toc.size;
        return chunk;
    }

    void writeDamb(const ManifestSpec& manifest, const std::filesystem::path& manifest_path) {
        const std::filesystem::path base_dir = manifest_path.parent_path();
        std::vector<ChunkBlob> chunks;
        chunks.push_back(buildImageChunk(manifest, base_dir));
        chunks.push_back(buildAtlasChunk(manifest));
        chunks.push_back(buildMapChunk(manifest));

        u64 cursor = damb::HEADER_SIZE;
        for (ChunkBlob& chunk : chunks) {
            chunk.toc.offset = cursor;
            cursor += chunk.toc.size;
            cursor += damb::PadTo8(cursor);
        }

        const u64 toc_offset = cursor;
        const u32 toc_count = static_cast<u32>(chunks.size());
        const u64 file_size = toc_offset + (static_cast<u64>(toc_count) * damb::TOC_ENTRY_SIZE);

        damb::Header header {};
        std::memcpy(header.magic, damb::MAGIC, 8);
        header.file_size = file_size;
        header.toc_offset = toc_offset;
        header.toc_count = toc_count;
        header.toc_entry_size = damb::TOC_ENTRY_SIZE;
        header.flags = 0;
        header.version = damb::VERSION;

        const std::filesystem::path output_path = base_dir / manifest.output_path;
        std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("Unable to open output file for writing: " + output_path.string());
        }

        out.write(reinterpret_cast<const char*>(&header), sizeof(header));
        if (!out) {
            throw std::runtime_error("Failed writing DAMB header.");
        }

        for (const ChunkBlob& chunk : chunks) {
            out.write(reinterpret_cast<const char*>(chunk.bytes.data()), static_cast<std::streamsize>(chunk.bytes.size()));
            if (!out) {
                throw std::runtime_error("Failed writing chunk payload.");
            }

            const u64 padding_size = damb::PadTo8(static_cast<u64>(out.tellp()));
            if (padding_size > 0) {
                const std::vector<char> zeroes(static_cast<std::size_t>(padding_size), '\0');
                out.write(zeroes.data(), static_cast<std::streamsize>(zeroes.size()));
                if (!out) {
                    throw std::runtime_error("Failed writing chunk alignment padding.");
                }
            }
        }

        for (const ChunkBlob& chunk : chunks) {
            out.write(reinterpret_cast<const char*>(&chunk.toc), sizeof(chunk.toc));
            if (!out) {
                throw std::runtime_error("Failed writing TOC entry.");
            }
        }

        if (static_cast<u64>(out.tellp()) != file_size) {
            throw std::runtime_error("Final file size mismatch while writing DAMB output.");
        }

        std::cout << "Wrote " << output_path.string() << " (" << file_size << " bytes).\n";
    }

    void printUsage() {
        std::cout << "Usage:\n"
                  << "  dambassador -c <manifest-file>\n"
                  << "  dambassador -x <damb-file>\n";
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            printUsage();
            return 1;
        }

        const std::string command = argv[1];

        if (command == "-c") {
            const std::filesystem::path manifest_path = argv[2];
            const ManifestSpec manifest = parseManifest(manifest_path);
            writeDamb(manifest, manifest_path);
            return 0;
        }

        if (command == "-x") {
            std::cout << "extract not implemented\n";
            return 0;
        }

        printUsage();
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "dambassador error: " << ex.what() << '\n';
        return 1;
    }
}
