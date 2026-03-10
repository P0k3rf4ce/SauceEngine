#pragma once

#include "app/modeling/Texture.hpp"
#include "app/modeling/PropertyValue.hpp"
#include <glm/glm.hpp>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>


namespace sauce {
    struct LogicalDevice;
}

namespace sauce {
namespace modeling {

struct MaterialProperties {
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    glm::vec3 emissiveFactor = glm::vec3(0.0f);
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;

    enum class AlphaMode {
        Opaque,
        Mask,
        Blend
    };

    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
};

// GPU-side UBO layout (std140-friendly)
struct MaterialUBO {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    glm::vec4 emissiveFactor_alphaCutoff; // xyz = emissive, w = alphaCutoff
};

class Material {
public:
    Material(const std::string& name = "");

    const std::string& getName() const { return name; }
    const MaterialProperties& getProperties() const { return properties; }
    MaterialProperties& getProperties() { return properties; }

    std::vector<vk::DescriptorBufferInfo> getDescriptorBufferInfos() const;
    std::vector<vk::DescriptorImageInfo>  getDescriptorImageInfos(
        const vk::raii::ImageView& defaultView,
        const vk::raii::Sampler& defaultSampler) const;

    std::shared_ptr<Texture> getTexture(TextureType type) const;
    void setTexture(TextureType type, std::shared_ptr<Texture> texture);
    bool hasTexture(TextureType type) const;

    const std::unordered_map<TextureType, std::shared_ptr<Texture>>&
    getTextures() const { return textures; }

    const std::unordered_map<std::string, PropertyValue>& getMetadata() const { return metadata; }
    void setMetadata(const std::string& key, const PropertyValue& value);
    bool hasMetadata(const std::string& key) const;

    // Vulkan resource management
    void initVulkanResources(
        const sauce::LogicalDevice& logicalDevice,
        vk::raii::PhysicalDevice& physicalDevice,
        vk::raii::CommandPool& commandPool,
        vk::raii::Queue& queue
    );

private:
    std::string name;
    MaterialProperties properties;

    std::unordered_map<TextureType, std::shared_ptr<Texture>> textures;
    std::unordered_map<std::string, PropertyValue> metadata;

    // Vulkan resources
    std::unique_ptr<vk::raii::Buffer> uniformBuffer;
    std::unique_ptr<vk::raii::DeviceMemory> uniformBufferMemory;

    // Helpers
    void updateUniformBuffer(const sauce::LogicalDevice& logicalDevice) const;
};

} // namespace modeling
} // namespace sauce