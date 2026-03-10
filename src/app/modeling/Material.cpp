#include "app/modeling/Material.hpp"
#include <app/LogicalDevice.hpp>
#include <app/BufferUtils.hpp>
#include <cstring>
#include <stdexcept>

namespace sauce {
namespace modeling {

Material::Material(const std::string& name)
    : name(name) {
}

std::shared_ptr<Texture> Material::getTexture(TextureType type) const {
    auto it = textures.find(type);
    if (it != textures.end()) {
        return it->second;
    }
    return nullptr;
}

void Material::setTexture(TextureType type, std::shared_ptr<Texture> texture) {
    textures[type] = texture;
}

bool Material::hasTexture(TextureType type) const {
    return textures.find(type) != textures.end();
}

void Material::setMetadata(const std::string& key, const PropertyValue& value) {
    metadata[key] = value;
}

bool Material::hasMetadata(const std::string& key) const {
    return metadata.find(key) != metadata.end();
}

// --- Vulkan resource management ---

void Material::updateUniformBuffer(const sauce::LogicalDevice& logicalDevice) const {
    if (!uniformBufferMemory) return;

    MaterialUBO ubo{};
    ubo.baseColorFactor = properties.baseColorFactor;
    ubo.metallicFactor = properties.metallicFactor;
    ubo.roughnessFactor = properties.roughnessFactor;
    ubo.normalScale = properties.normalScale;
    ubo.occlusionStrength = properties.occlusionStrength;
    ubo.emissiveFactor_alphaCutoff = glm::vec4(properties.emissiveFactor, properties.alphaCutoff);

    void* data = uniformBufferMemory->mapMemory(0, sizeof(MaterialUBO));
    std::memcpy(data, &ubo, sizeof(MaterialUBO));
    uniformBufferMemory->unmapMemory();
}

void Material::initVulkanResources(
    const sauce::LogicalDevice& logicalDevice,
    vk::raii::PhysicalDevice& physicalDevice,
    vk::raii::CommandPool& commandPool,
    vk::raii::Queue& queue
) {
    // 1. Initialize Vulkan resources on all child Textures
    for (auto& [type, texture] : textures) {
        if (texture) {
            texture->initVulkanResources(logicalDevice, physicalDevice, commandPool, queue);
        }
    }

    // 2. Create the uniform buffer for material constants
    uniformBuffer = std::make_unique<vk::raii::Buffer>(nullptr);
    uniformBufferMemory = std::make_unique<vk::raii::DeviceMemory>(nullptr);

    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice,
        sizeof(MaterialUBO),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        *uniformBuffer, *uniformBufferMemory
    );

    // 3. Upload initial material data
    updateUniformBuffer(logicalDevice);
}

std::vector<vk::DescriptorBufferInfo> Material::getDescriptorBufferInfos() const {
    std::vector<vk::DescriptorBufferInfo> infos;
    if (uniformBuffer) {
        infos.push_back(vk::DescriptorBufferInfo{
            .buffer = **uniformBuffer,
            .offset = 0,
            .range = sizeof(MaterialUBO)
        });
    }
    return infos;
}

std::vector<vk::DescriptorImageInfo> Material::getDescriptorImageInfos(
    const vk::raii::ImageView& defaultView,
    const vk::raii::Sampler& defaultSampler) const
{
    std::vector<vk::DescriptorImageInfo> infos;
    // Order: Albedo, Normal, MetallicRoughness, Emissive, AO
    std::array<TextureType, 5> types = {
        TextureType::BaseColor,
        TextureType::Normal,
        TextureType::MetallicRoughness,
        TextureType::Emissive,
        TextureType::Occlusion
    };

    for (auto type : types) {
        auto it = textures.find(type);
        if (it != textures.end() && it->second && it->second->hasGPUData()) {
            infos.push_back(it->second->getDescriptorInfo());
        } else {
            infos.push_back(vk::DescriptorImageInfo{
                .sampler = *defaultSampler,
                .imageView = *defaultView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            });
        }
    }
    return infos;
}

} // namespace modeling
} // namespace sauce
