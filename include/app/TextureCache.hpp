#pragma once

#include <app/Texture.hpp>

#include <stb_image.h>

#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>

namespace sauce {

class TextureCache {
public:
  static TextureCache& getInstance() {
    static TextureCache instance;
    return instance;
  }

  TexturePtr getTexture(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto it = cache.find(filepath);
    if (it != cache.end()) {
      return it->second;
    }

    auto texture = loadTexture(filepath);
    if (texture) {
      cache[filepath] = texture;
    }
    return texture;
  }

  TexturePtr getTextureOrDefault(const std::string& filepath, TexturePtr defaultTexture) {
    auto texture = getTexture(filepath);
    return texture ? texture : defaultTexture;
  }

  bool hasTexture(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache.find(filepath) != cache.end();
  }

  void removeTexture(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.erase(filepath);
  }

  void clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.clear();
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache.size();
  }

  TexturePtr createFromMemory(const std::string& name, int width, int height, int channels, const uint8_t* data) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto it = cache.find(name);
    if (it != cache.end()) {
      return it->second;
    }

    size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
    std::vector<uint8_t> pixelData(data, data + dataSize);

    auto texture = std::make_shared<Texture>(
        nextTextureId++,
        width,
        height,
        channels,
        std::move(pixelData),
        name
    );

    cache[name] = texture;
    return texture;
  }

private:
  TextureCache() = default;
  ~TextureCache() = default;
  TextureCache(const TextureCache&) = delete;
  TextureCache& operator=(const TextureCache&) = delete;

  TexturePtr loadTexture(const std::string& filepath) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

    if (!data) {
      return nullptr;
    }

    size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
    std::vector<uint8_t> pixelData(data, data + dataSize);
    stbi_image_free(data);

    return std::make_shared<Texture>(
        nextTextureId++,
        width,
        height,
        channels,
        std::move(pixelData),
        filepath
    );
  }

  mutable std::mutex cacheMutex;
  std::unordered_map<std::string, TexturePtr> cache;
  std::atomic<uint32_t> nextTextureId{1};
};

}
