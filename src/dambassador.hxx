#ifndef DAMBASSADOR_HXX_INCLUDED
#define DAMBASSADOR_HXX_INCLUDED

#include "format_damb.hxx"

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace amb {
    namespace damb {
        struct ImageSpec {
            u16 id = 0;
            std::filesystem::path file_path;
            u32 width = 0;
            u32 height = 0;
            ImageFormat format = ImageFormat::png;
        };

        struct AtlasSpec {
            u16 id = 0;
            u16 image_id = 0;
            std::vector<AtlasRecord> records;
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
    }

    class Dambassador {
      public:
        void create(const std::filesystem::path& manifest_path) const;
        void extract(const std::filesystem::path& damb_path) const;
        void inspect(const std::filesystem::path& damb_path) const;

        static void printUsage(std::ostream& out);

      private:
        struct ChunkBlob {
            damb::TocEntry toc {};
            std::vector<u8> bytes;
        };

        damb::ManifestSpec parseManifest(const std::filesystem::path& manifest_path) const;
        damb::ImageFormat parseImageFormat(const std::string& value, std::size_t line_number) const;

        std::vector<u8> readFileBytes(const std::filesystem::path& path) const;

        ChunkBlob buildImageChunk(const damb::ManifestSpec& manifest, const std::filesystem::path& base_dir) const;
        ChunkBlob buildAtlasChunk(const damb::ManifestSpec& manifest) const;
        ChunkBlob buildMapChunk(const damb::ManifestSpec& manifest) const;

        void writeDamb(const damb::ManifestSpec& manifest, const std::filesystem::path& manifest_path) const;
    };
}

#endif
