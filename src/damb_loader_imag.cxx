#include "damb_loader.hxx"
#include "damb_imag.hxx"

#include "utility_binary.hxx"

#include <SDL3_image/SDL_image.h>

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    namespace damb = amb::damb;
}

ImageRuntime DambLoader::loadImageRuntime(std::ifstream& stream, const damb::TocEntry& image_entry, SDL_Renderer* renderer) const {
    if (renderer == nullptr) {
        throw std::runtime_error("Cannot load IMAG chunk without a valid SDL_Renderer.");
    }

    stream.seekg(static_cast<std::streamoff>(image_entry.offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to IMAG chunk.");
    }

    const damb::ImageChunkHeader image_header = amb::utility::readPod<damb::ImageChunkHeader>(stream, "IMAG header");
    if (!amb::utility::chunkTypeEquals(image_header.header.type, damb::CL_IMAGE)) {
        throw std::runtime_error("TOC IMAG entry points to a non-IMAG chunk.");
    }

    if (image_header.header.id != image_entry.id) {
        throw std::runtime_error("TOC IMAG entry id does not match IMAG chunk header id.");
    }

    if (image_header.format != damb::ImageFormat::png) {
        throw std::runtime_error("Only PNG IMAG chunk format is supported.");
    }

    if (image_header.size == 0) {
        throw std::runtime_error("IMAG chunk has empty image payload.");
    }

    if (image_header.size > static_cast<u64>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("IMAG payload is too large for this platform.");
    }

    const u64 expected_chunk_size = static_cast<u64>(damb::IMAG_HEADER_SIZE) + image_header.size;
    if (image_entry.size < expected_chunk_size) {
        throw std::runtime_error("IMAG TOC size is smaller than declared IMAG payload.");
    }

    std::vector<u8> image_blob(static_cast<std::size_t>(image_header.size));
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

    if (!SDL_SetTextureScaleMode(image_runtime.texture.get(), SDL_SCALEMODE_NEAREST)) {
        throw std::runtime_error(std::string("Failed to set texture scale mode: ") + SDL_GetError());
    }

    return image_runtime;
}
