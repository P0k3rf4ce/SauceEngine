#include "app/modeling/TextureCache.hpp"
#include <sstream>
#include <iomanip>

namespace sauce {
namespace modeling {

TextureCache::TextureCache() {
    initializeDefaultTextures();
}

std::shared_ptr<Texture> TextureCache::getTexture(const std::string& path, TextureType type, bool sRGB) {
    // Check if texture is already in cache
    auto it = cache.find(path);
    if (it != cache.end()) {
        return it->second;
    }

    // Create new texture and add to cache
    auto texture = std::make_shared<Texture>(path, type, sRGB);
    cache[path] = texture;
    return texture;
}

std::shared_ptr<Texture> TextureCache::getHDRTexture(const std::string& path) {
    auto it = cache.find(path);
    if (it != cache.end()) {
        return it->second;
    }

    auto texture = std::make_shared<Texture>(path, TextureType::EnvironmentMapHDR, false);
    cache[path] = texture;
    return texture;
}

std::shared_ptr<Texture> TextureCache::getEmbeddedTexture(const std::vector<unsigned char>& data,
                                                           int width, int height, int channels,
                                                           TextureType type, bool sRGB) {
    // Generate hash-based key for embedded texture
    std::string key = generateEmbeddedKey(data);

    // Check if texture is already in cache
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    // Create new embedded texture and add to cache
    auto texture = std::make_shared<Texture>(data, width, height, channels, type, sRGB, key);
    cache[key] = texture;
    return texture;
}

std::shared_ptr<Texture> TextureCache::getDefaultTexture(TextureType type) {
    auto it = defaultTextures.find(type);
    if (it != defaultTextures.end()) {
        return it->second;
    }

    // Fallback to BaseColor default if specific type not found
    return defaultTextures[TextureType::BaseColor];
}

void TextureCache::clear() {
    cache.clear();
    // Don't clear default textures - they're always needed
}

std::string TextureCache::generateEmbeddedKey(const std::vector<unsigned char>& data) {
    // Simple hash: use first 16 bytes + size
    // For better hash, could use std::hash or a proper hash function
    std::stringstream ss;
    ss << "embedded_";

    size_t hashSize = std::min(data.size(), size_t(16));
    for (size_t i = 0; i < hashSize; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }

    ss << "_" << std::dec << data.size();
    return ss.str();
}

void TextureCache::initializeDefaultTextures() {
    // Default white texture for BaseColor
    {
        std::vector<unsigned char> whitePixel = {255, 255, 255, 255};
        defaultTextures[TextureType::BaseColor] = std::make_shared<Texture>(
            whitePixel, 1, 1, 4, TextureType::BaseColor, true, "default_white"
        );
    }

    // Default flat normal (pointing up: 0, 0, 1 -> 128, 128, 255 in RGB)
    {
        std::vector<unsigned char> normalPixel = {128, 128, 255, 255};
        defaultTextures[TextureType::Normal] = std::make_shared<Texture>(
            normalPixel, 1, 1, 4, TextureType::Normal, false, "default_normal"
        );
    }

    // Default metallic-roughness (non-metallic, medium roughness)
    // R=occlusion(1.0), G=roughness(0.5), B=metallic(0.0)
    {
        std::vector<unsigned char> metallicRoughnessPixel = {255, 128, 0, 255};
        defaultTextures[TextureType::MetallicRoughness] = std::make_shared<Texture>(
            metallicRoughnessPixel, 1, 1, 4, TextureType::MetallicRoughness, false, "default_metallic_roughness"
        );
    }

    // Default occlusion (no occlusion = white)
    {
        std::vector<unsigned char> occlusionPixel = {255, 255, 255, 255};
        defaultTextures[TextureType::Occlusion] = std::make_shared<Texture>(
            occlusionPixel, 1, 1, 4, TextureType::Occlusion, false, "default_occlusion"
        );
    }

    // Default emissive (no emission = black)
    {
        std::vector<unsigned char> emissivePixel = {0, 0, 0, 255};
        defaultTextures[TextureType::Emissive] = std::make_shared<Texture>(
            emissivePixel, 1, 1, 4, TextureType::Emissive, true, "default_emissive"
        );
    }
}

} // namespace modeling
} // namespace sauce
