#pragma once

#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

namespace sauce {
namespace modeling {

enum class TextureType {
    BaseColor,
    Normal,
    MetallicRoughness,
    Occlusion,
    Emissive,
    Unknown
};

class Texture {
public:
    // Constructor for external file path
    Texture(const std::string& path, TextureType type, bool sRGB);

    // Constructor for embedded texture data
    Texture(const std::vector<unsigned char>& data, int width, int height, int channels,
            TextureType type, bool sRGB, const std::string& name = "embedded");

    // Getters
    const std::string& getPath() const { return path; }
    TextureType getType() const { return type; }
    bool isSRGB() const { return sRGB; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getChannels() const { return channels; }
    bool isEmbedded() const { return embedded; }
    bool hasGPUData() const { return image != nullptr; }

    // Get CPU-side texture data (loads from file if necessary)
    const std::vector<unsigned char>& getData();

    // Optional GPU upload (Phase 6)
    // void uploadToGPU(vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice);

private:
    std::string path;
    TextureType type;
    bool sRGB;
    bool embedded;

    // CPU-side data
    int width;
    int height;
    int channels;
    std::vector<unsigned char> data;
    bool dataLoaded;

    // GPU resources (optional, for Phase 6)
    std::unique_ptr<vk::raii::Image> image;
    std::unique_ptr<vk::raii::ImageView> imageView;
    std::unique_ptr<vk::raii::DeviceMemory> imageMemory;
    std::unique_ptr<vk::raii::Sampler> sampler;

    // Helper to load image from file
    void loadFromFile();
};

} // namespace modeling
} // namespace sauce
