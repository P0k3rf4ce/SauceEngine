#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace sauce {

struct Texture {
  uint32_t id = 0;
  int width = 0;
  int height = 0;
  int numChannels = 0;
  std::vector<uint8_t> data;
  std::string filepath;

  Texture() = default;

  Texture(uint32_t texId, int w, int h, int channels, std::vector<uint8_t> pixelData, std::string path = "")
      : id(texId)
      , width(w)
      , height(h)
      , numChannels(channels)
      , data(std::move(pixelData))
      , filepath(std::move(path)) {}

  size_t getDataSize() const {
    return static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(numChannels);
  }

  bool isValid() const {
    return width > 0 && height > 0 && numChannels > 0 && !data.empty();
  }
};

using TexturePtr = std::shared_ptr<Texture>;

}
