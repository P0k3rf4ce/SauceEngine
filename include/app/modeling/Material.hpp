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
    // PBR Metallic-Roughness properties
    glm::vec4 baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    // Additional properties
    glm::vec3 emissiveFactor = glm::vec3(0.0f, 0.0f, 0.0f);
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;

    // Alpha mode
    enum class AlphaMode {
        Opaque,
        Mask,
        Blend
    };
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;

    // Double-sided
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

struct MaterialDescriptorInfo {
    vk::DescriptorBufferInfo bufferInfo;
    std::vector<vk::DescriptorImageInfo> imageInfos;
};

class Material {
public:
    Material(const std::string& name = "");

    // Getters
    const std::string& getName() const { return name; }
    const MaterialProperties& getProperties() const { return properties; }
    MaterialProperties& getProperties() { return properties; }

    // Texture access
    std::shared_ptr<Texture> getTexture(TextureType type) const;
    void setTexture(TextureType type, std::shared_ptr<Texture> texture);
    bool hasTexture(TextureType type) const;
    const std::unordered_map<TextureType, std::shared_ptr<Texture>>& getTextures() const { return textures; }

    // Metadata access (for GLTF extensions)
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
    MaterialDescriptorInfo getDescriptorInfo() const;

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