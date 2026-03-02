#pragma once

#include "app/modeling/Texture.hpp"
#include <unordered_map>
#include <memory>
#include <string>

namespace sauce {
namespace modeling {

class TextureCache {
public:
    TextureCache();

    // Get or create texture from file path
    std::shared_ptr<Texture> getTexture(const std::string& path, TextureType type, bool sRGB);

    // Load an HDR environment map (.hdr file). HDR detection and stbi_loadf
    // happen inside Texture::loadFromFile; this method sets the correct type.
    std::shared_ptr<Texture> getHDRTexture(const std::string& path);

    // Get or create texture from embedded data (uses hash-based key)
    std::shared_ptr<Texture> getEmbeddedTexture(const std::vector<unsigned char>& data,
                                                 int width, int height, int channels,
                                                 TextureType type, bool sRGB);

    // Get default textures (1x1 pixels)
    std::shared_ptr<Texture> getDefaultTexture(TextureType type);

    // Clear the cache
    void clear();

    // Get cache statistics
    size_t getCacheSize() const { return cache.size(); }

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> cache;
    std::unordered_map<TextureType, std::shared_ptr<Texture>> defaultTextures;

    // Generate a hash key for embedded textures
    std::string generateEmbeddedKey(const std::vector<unsigned char>& data);

    // Initialize default textures
    void initializeDefaultTextures();
};

} // namespace modeling
} // namespace sauce
