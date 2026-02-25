#include "dambassador.hxx"

#include "utility_binary.hxx"
#include "utility_parse.hxx"
#include "utility_string.hxx"

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace amb {
    namespace {
        enum class ManifestParseState {
            top,
            atlas,
            map,
            rows,
        };

        void parseAtlasTileRecord(
            damb::AtlasRecord& record,
            bool& has_rect,
            const std::vector<std::string>& tokens,
            std::size_t line_number
        ) {
            for (std::size_t i = 2; i < tokens.size(); i++) {
                const auto [key, value] = utility::parseKeyValue(tokens[i], line_number);

                if (key == "rect") {
                    const std::vector<std::string> values = utility::split(value, ',');
                    if (values.size() != 4) {
                        throw std::runtime_error("Line " + std::to_string(line_number) + ": rect requires x,y,w,h.");
                    }
                    record.src_x = utility::parseUnsigned16(utility::trim(values[0]), line_number, "tile rect x");
                    record.src_y = utility::parseUnsigned16(utility::trim(values[1]), line_number, "tile rect y");
                    record.src_w = utility::parseUnsigned16(utility::trim(values[2]), line_number, "tile rect w");
                    record.src_h = utility::parseUnsigned16(utility::trim(values[3]), line_number, "tile rect h");
                    has_rect = true;
                    continue;
                }

                if (key == "flags") {
                    record.flags = utility::parseUnsigned32(value, line_number, "tile flags");
                    continue;
                }

                if (key == "anchor") {
                    const std::vector<std::string> values = utility::split(value, ',');
                    if (values.size() != 2) {
                        throw std::runtime_error("Line " + std::to_string(line_number) + ": anchor requires x,y.");
                    }
                    record.anchor_x = utility::parseSigned16(utility::trim(values[0]), line_number, "tile anchor x");
                    record.anchor_y = utility::parseSigned16(utility::trim(values[1]), line_number, "tile anchor y");
                }
            }
        }

        void validateManifest(const damb::ManifestSpec& manifest, ManifestParseState state, bool saw_manifest_header) {
            if (!saw_manifest_header) {
                throw std::runtime_error("Manifest is empty or missing `damb_manifest 1` header.");
            }
            if (state != ManifestParseState::top) {
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
        }

        damb::ImageFormat parseImageFormatValue(const std::string& value, std::size_t line_number) {
            if (value == "png") {
                return damb::ImageFormat::png;
            }

            throw std::runtime_error("Line " + std::to_string(line_number) + ": unsupported image format: " + value);
        }

        class ManifestParser {
        public:
            damb::ManifestSpec parse(const std::filesystem::path& manifest_path) {
                std::ifstream stream(manifest_path);
                if (!stream.is_open()) {
                    throw std::runtime_error("Unable to open manifest file: " + manifest_path.string());
                }

                std::string line;
                while (std::getline(stream, line)) {
                    m_line_number++;
                    const std::string cleaned = utility::trim(line);
                    if (cleaned.empty() || cleaned[0] == ';') {
                        continue;
                    }

                    const std::vector<std::string> tokens = utility::splitWhitespace(cleaned);
                    if (!m_saw_manifest_header) {
                        parseHeader(tokens);
                        continue;
                    }

                    if (m_state == ManifestParseState::rows) {
                        parseRowsContent(cleaned, tokens);
                        continue;
                    }

                    parseStatement(tokens);
                }

                validateManifest(m_manifest, m_state, m_saw_manifest_header);
                return m_manifest;
            }

        private:
            void parseHeader(const std::vector<std::string>& tokens) {
                if (tokens.size() != 2 || tokens[0] != "damb_manifest") {
                    throw std::runtime_error(
                        "Line " + std::to_string(m_line_number) + ": first non-comment line must be `damb_manifest 1`."
                    );
                }

                const u64 version = utility::parseUnsigned(tokens[1], m_line_number, "manifest version");
                if (version != 1) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": only manifest version 1 is supported.");
                }

                m_saw_manifest_header = true;
            }

            void parseRowsContent(const std::string& cleaned, const std::vector<std::string>& tokens) {
                if (tokens.size() == 1 && tokens[0] == "endrows") {
                    m_state = ManifestParseState::map;
                    return;
                }

                const std::vector<std::string> row_tokens = utility::split(cleaned, '|');
                if (row_tokens.size() != m_manifest.map.width) {
                    throw std::runtime_error(
                        "Line " + std::to_string(m_line_number) + ": row width mismatch; expected " +
                        std::to_string(m_manifest.map.width) + " values separated by '|'."
                    );
                }

                for (const std::string& cell_token_raw : row_tokens) {
                    const std::string cell_token = utility::trim(cell_token_raw);
                    const u16 tile_id = utility::parseUnsigned16(cell_token, m_line_number, "map tile id");
                    m_manifest.map.tile_ids.push_back(tile_id);
                }
            }

            void parseStatement(const std::vector<std::string>& tokens) {
                const std::string& keyword = tokens[0];
                if (keyword == "output") { parseOutput(tokens); return; }
                if (keyword == "image") { parseImage(tokens); return; }
                if (keyword == "atlas") { parseAtlasStart(tokens); return; }
                if (keyword == "tile") { parseTile(tokens); return; }
                if (keyword == "endatlas") { parseAtlasEnd(tokens); return; }
                if (keyword == "map") { parseMapStart(tokens); return; }
                if (keyword == "rows") { parseRowsStart(tokens); return; }
                if (keyword == "endmap") { parseMapEnd(tokens); return; }

                throw std::runtime_error("Line " + std::to_string(m_line_number) + ": unknown statement: " + keyword);
            }

            void parseOutput(const std::vector<std::string>& tokens) {
                if (m_state != ManifestParseState::top || tokens.size() != 2) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": output line must be `output <path>` at top scope.");
                }

                m_manifest.output_path = tokens[1];
                m_manifest.has_output = true;
            }

            void parseImage(const std::vector<std::string>& tokens) {
                if (m_state != ManifestParseState::top || tokens.size() != 6) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": image line must be `image <id> <path> <width> <height> <format>`.");
                }

                m_manifest.image.id = utility::parseUnsigned16(tokens[1], m_line_number, "image id");
                m_manifest.image.file_path = tokens[2];
                m_manifest.image.width = utility::parseUnsigned32(tokens[3], m_line_number, "image width");
                m_manifest.image.height = utility::parseUnsigned32(tokens[4], m_line_number, "image height");
                m_manifest.image.format = parseImageFormatValue(tokens[5], m_line_number);
                m_manifest.has_image = true;
            }

            void parseAtlasStart(const std::vector<std::string>& tokens) {
                if (m_state != ManifestParseState::top || tokens.size() != 3) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": atlas line must be `atlas <id> image=<image_id>`.");
                }

                m_manifest.atlas = damb::AtlasSpec {};
                m_manifest.atlas.id = utility::parseUnsigned16(tokens[1], m_line_number, "atlas id");

                const auto [key, value] = utility::parseKeyValue(tokens[2], m_line_number);
                if (key != "image") {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": atlas line must include image=<image_id>.");
                }

                m_manifest.atlas.image_id = utility::parseUnsigned16(value, m_line_number, "atlas image_id");
                m_manifest.atlas.records.clear();
                m_manifest.has_atlas = true;
                m_state = ManifestParseState::atlas;
            }

            void parseTile(const std::vector<std::string>& tokens) {
                if (m_state != ManifestParseState::atlas) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": tile entry is only valid inside atlas block.");
                }
                if (tokens.size() < 3) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": tile entry must include id and rect.");
                }

                damb::AtlasRecord record {};
                record.id = utility::parseUnsigned16(tokens[1], m_line_number, "tile id");
                bool has_rect = false;
                parseAtlasTileRecord(record, has_rect, tokens, m_line_number);

                if (!has_rect) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": tile entry is missing rect=x,y,w,h.");
                }

                m_manifest.atlas.records.push_back(record);
            }

            void parseAtlasEnd(const std::vector<std::string>& tokens) {
                if (m_state != ManifestParseState::atlas || tokens.size() != 1) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": unexpected endatlas.");
                }

                m_state = ManifestParseState::top;
            }

            void parseMapStart(const std::vector<std::string>& tokens) {
                if (m_state != ManifestParseState::top || tokens.size() != 6) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": map line must be `map <id> atlas=<id> width=<w> height=<h> z=<z>`." );
                }

                m_manifest.map = damb::MapSpec {};
                m_manifest.map.id = utility::parseUnsigned16(tokens[1], m_line_number, "map id");

                for (std::size_t i = 2; i < tokens.size(); i++) {
                    const auto [key, value] = utility::parseKeyValue(tokens[i], m_line_number);
                    if (key == "atlas") {
                        m_manifest.map.atlas_id = utility::parseUnsigned16(value, m_line_number, "map atlas_id");
                    } else if (key == "width") {
                        m_manifest.map.width = utility::parseUnsigned32(value, m_line_number, "map width");
                    } else if (key == "height") {
                        m_manifest.map.height = utility::parseUnsigned32(value, m_line_number, "map height");
                    } else if (key == "z") {
                        m_manifest.map.z = utility::parseSigned32(value, m_line_number, "map z");
                    } else {
                        throw std::runtime_error("Line " + std::to_string(m_line_number) + ": unknown map field: " + key);
                    }
                }

                m_manifest.map.tile_ids.clear();
                m_manifest.has_map = true;
                m_state = ManifestParseState::map;
            }

            void parseRowsStart(const std::vector<std::string>& tokens) {
                if (m_state != ManifestParseState::map || tokens.size() != 1) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": rows block must be inside map.");
                }

                m_state = ManifestParseState::rows;
            }

            void parseMapEnd(const std::vector<std::string>& tokens) {
                if (m_state != ManifestParseState::map || tokens.size() != 1) {
                    throw std::runtime_error("Line " + std::to_string(m_line_number) + ": unexpected endmap.");
                }

                m_state = ManifestParseState::top;
            }

            damb::ManifestSpec m_manifest {};
            ManifestParseState m_state = ManifestParseState::top;
            std::size_t m_line_number = 0;
            bool m_saw_manifest_header = false;
        };
    }

    void Dambassador::create(const std::filesystem::path& manifest_path) const {
        const damb::ManifestSpec manifest = parseManifest(manifest_path);
        writeDamb(manifest, manifest_path);
    }

    void Dambassador::extract(const std::filesystem::path& damb_path) const {
        std::cout << "extract not implemented for: " << damb_path.string() << '\n';
    }

    void Dambassador::inspect(const std::filesystem::path& damb_path) const {
        std::cout << "inspect not implemented for: " << damb_path.string() << '\n';
    }

    void Dambassador::printUsage(std::ostream& out) {
        out << "Usage:\n"
            << "  dambassador -c <manifest-file>\n"
            << "  dambassador -x <damb-file>\n"
            << "  dambassador -i <damb-file>\n";
    }

    damb::ImageFormat Dambassador::parseImageFormat(const std::string& value, std::size_t line_number) const {
        if (value == "png") {
            return damb::ImageFormat::png;
        }

        throw std::runtime_error("Line " + std::to_string(line_number) + ": unsupported image format: " + value);
    }

    damb::ManifestSpec Dambassador::parseManifest(const std::filesystem::path& manifest_path) const {
        ManifestParser parser;
        return parser.parse(manifest_path);
    }

    std::vector<u8> Dambassador::readFileBytes(const std::filesystem::path& path) const {
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

    Dambassador::ChunkBlob Dambassador::buildImageChunk(
        const damb::ManifestSpec& manifest,
        const std::filesystem::path& base_dir
    ) const {
        ChunkBlob chunk;

        const std::filesystem::path image_path = base_dir / manifest.image.file_path;
        const std::vector<u8> image_bytes = readFileBytes(image_path);

        damb::ImageChunkHeader header {};
        std::memcpy(header.header.type, damb::CL_IMAGE, amb::data::CHUNK_TYPE_LENGTH);
        header.header.id = manifest.image.id;
        header.size = static_cast<u64>(image_bytes.size());
        header.width = manifest.image.width;
        header.height = manifest.image.height;
        header.format = manifest.image.format;

        utility::appendPod(chunk.bytes, header);
        chunk.bytes.insert(chunk.bytes.end(), image_bytes.begin(), image_bytes.end());

        std::memcpy(chunk.toc.type, damb::CL_IMAGE, amb::data::CHUNK_TYPE_LENGTH);
        chunk.toc.id = manifest.image.id;
        chunk.toc.size = chunk.bytes.size();
        chunk.toc.uncompressed_size = chunk.toc.size;
        return chunk;
    }

    Dambassador::ChunkBlob Dambassador::buildAtlasChunk(const damb::ManifestSpec& manifest) const {
        ChunkBlob chunk;

        damb::AtlasChunkHeader header {};
        std::memcpy(header.header.type, damb::CL_ATLAS, amb::data::CHUNK_TYPE_LENGTH);
        header.header.id = manifest.atlas.id;
        header.asset_count = static_cast<u32>(manifest.atlas.records.size());
        header.image_id = manifest.atlas.image_id;

        utility::appendPod(chunk.bytes, header);
        for (const damb::AtlasRecord& record : manifest.atlas.records) {
            utility::appendPod(chunk.bytes, record);
        }

        std::memcpy(chunk.toc.type, damb::CL_ATLAS, amb::data::CHUNK_TYPE_LENGTH);
        chunk.toc.id = manifest.atlas.id;
        chunk.toc.size = chunk.bytes.size();
        chunk.toc.uncompressed_size = chunk.toc.size;
        return chunk;
    }

    Dambassador::ChunkBlob Dambassador::buildMapChunk(const damb::ManifestSpec& manifest) const {
        ChunkBlob chunk;

        damb::MapLayerChunkHeader header {};
        std::memcpy(header.header.type, damb::CL_MAP_LAYER, amb::data::CHUNK_TYPE_LENGTH);
        header.header.id = manifest.map.id;
        header.width = manifest.map.width;
        header.height = manifest.map.height;
        header.z = manifest.map.z;
        header.atlas_id = manifest.map.atlas_id;
        header.encoding = damb::MapEncoding::raw;

        utility::appendPod(chunk.bytes, header);

        for (const u16 atlas_record_index : manifest.map.tile_ids) {
            damb::MapCell cell {};
            cell.id = 0;
            cell.atlas_record_index = atlas_record_index;
            utility::appendPod(chunk.bytes, cell);
        }

        std::memcpy(chunk.toc.type, damb::CL_MAP_LAYER, amb::data::CHUNK_TYPE_LENGTH);
        chunk.toc.id = manifest.map.id;
        chunk.toc.size = chunk.bytes.size();
        chunk.toc.uncompressed_size = chunk.toc.size;
        return chunk;
    }

    void Dambassador::writeDamb(const damb::ManifestSpec& manifest, const std::filesystem::path& manifest_path) const {
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
        std::memcpy(header.magic, damb::MAGIC, amb::data::MAGIC_LENGTH);
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
}
