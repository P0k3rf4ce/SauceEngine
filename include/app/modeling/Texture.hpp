#pragma once

#include <string>
#include <vector>
#include <memory>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

namespace sauce {
    struct LogicalDevice;
}

namespace sauce {
namespace modeling {

enum class TextureType {
    BaseColor,
    Normal,
    MetallicRoughness,
    Occlusion,
    Emissive,
    EnvironmentMapHDR,
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
    bool isHDR() const { return hdr; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getChannels() const { return channels; }
    bool isEmbedded() const { return embedded; }
    bool hasGPUData() const { return image != nullptr; }

    // Get CPU-side texture data (loads from file if necessary)
    const std::vector<unsigned char>& getData();
    const std::vector<float>& getHDRData();

    // Initialize Vulkan GPU resources
    void initVulkanResources(
        const sauce::LogicalDevice& logicalDevice,
        vk::raii::PhysicalDevice& physicalDevice,
        vk::raii::CommandPool& commandPool,
        vk::raii::Queue& queue
    );

    // Get descriptor info
    vk::DescriptorImageInfo getDescriptorInfo() const;

private:
    std::string path;
    TextureType type;
    bool sRGB;
    bool hdr;
    bool embedded;

    // CPU-side data
    int width;
    int height;
    int channels;
    std::vector<unsigned char> data;
    std::vector<float> hdrData;
    bool dataLoaded;

    // GPU resources (optional, for Phase 6)
    std::unique_ptr<vk::raii::Image> image;
    std::unique_ptr<vk::raii::ImageView> imageView;
    std::unique_ptr<vk::raii::DeviceMemory> imageMemory;
    std::unique_ptr<vk::raii::Sampler> sampler;

    void loadFromFile();
    void loadFromFileHDR();
    void createDefaultPixels();
};

} // namespace modeling
} // namespace sauce
