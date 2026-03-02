#ifndef DAMBASSADOR_HXX_INCLUDED
#define DAMBASSADOR_HXX_INCLUDED

#include "damb_spec.hxx"

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace amb {
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
        ChunkBlob buildEntityChunk(const damb::ManifestSpec& manifest) const;

        void writeDamb(const damb::ManifestSpec& manifest, const std::filesystem::path& manifest_path) const;
    };
}

#endif
