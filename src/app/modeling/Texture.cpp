#include "app/modeling/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace sauce {
namespace modeling {

Texture::Texture(const std::string& path, TextureType type, bool sRGB)
    : path(path)
    , type(type)
    , sRGB(sRGB)
    , embedded(false)
    , width(0)
    , height(0)
    , channels(0)
    , dataLoaded(false) {
}

Texture::Texture(const std::vector<unsigned char>& data, int width, int height, int channels,
                 TextureType type, bool sRGB, const std::string& name)
    : path(name)
    , type(type)
    , sRGB(sRGB)
    , embedded(true)
    , width(width)
    , height(height)
    , channels(channels)
    , data(data)
    , dataLoaded(true) {
}

const std::vector<unsigned char>& Texture::getData() {
    if (!dataLoaded) {
        loadFromFile();
    }
    return data;
}

void Texture::loadFromFile() {
    if (dataLoaded) {
        return;
    }

    if (embedded) {
        return;
    }

    int w, h, c;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &c, STBI_rgb_alpha);

    if (!pixels) {
        // Create a default 1x1 texture based on type
        width = 1;
        height = 1;
        channels = 4;
        data.resize(4);

        if (type == TextureType::Normal) {
            // Flat normal pointing up (128, 128, 255, 255) -> (0, 0, 1) in tangent space
            data[0] = 128;
            data[1] = 128;
            data[2] = 255;
            data[3] = 255;
        } else if (type == TextureType::MetallicRoughness || type == TextureType::Occlusion) {
            // Black (non-metallic, smooth, no occlusion)
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = 255;
        } else {
            // White default for BaseColor and Emissive
            data[0] = 255;
            data[1] = 255;
            data[2] = 255;
            data[3] = 255;
        }

        dataLoaded = true;
        return;
    }

    width = w;
    height = h;
    channels = 4; // stbi_load with STBI_rgb_alpha always returns 4 channels

    // Copy pixel data
    size_t dataSize = width * height * channels;
    data.resize(dataSize);
    std::memcpy(data.data(), pixels, dataSize);

    stbi_image_free(pixels);
    dataLoaded = true;
}

} // namespace modeling
} // namespace sauce
