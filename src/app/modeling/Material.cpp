#include "app/modeling/Material.hpp"
#include <app/LogicalDevice.hpp>
#include <app/BufferUtils.hpp>
#include <cstring>
#include <stdexcept>

namespace sauce {
namespace modeling {

std::unique_ptr<vk::raii::DescriptorSetLayout> Material::descriptorSetLayout = nullptr;

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

void Material::initDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice) {
    if (descriptorSetLayout) return;

    std::array<vk::DescriptorSetLayoutBinding, 6> materialBindings;
    // Albedo, Normal, MetallicRoughness, Emissive, AO textures
    for (uint32_t i = 0; i < 5; ++i) {
        materialBindings[i] = {
            .binding = i,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };
    }
    // Material Properties UBO
    materialBindings[5] = {
        .binding = 5,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    vk::DescriptorSetLayoutCreateInfo materialDsLayoutInfo {
        .bindingCount = static_cast<uint32_t>(materialBindings.size()),
        .pBindings = materialBindings.data(),
    };
    descriptorSetLayout = std::make_unique<vk::raii::DescriptorSetLayout>(*logicalDevice, materialDsLayoutInfo);
}

const vk::raii::DescriptorSetLayout& Material::getDescriptorSetLayout() {
    if (!descriptorSetLayout) {
        throw std::runtime_error("Material descriptor set layout not initialized!");
    }
    return *descriptorSetLayout;
}

void Material::cleanup() {
    descriptorSetLayout.reset();
}

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
    vk::raii::Queue& queue,
    const vk::raii::DescriptorPool& pool,
    const vk::raii::ImageView& defaultView,
    const vk::raii::Sampler& defaultSampler
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

    // 4. Allocate descriptor set
    vk::DescriptorSetLayout layoutHandle = *getDescriptorSetLayout();
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool = *pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layoutHandle,
    };

    auto sets = logicalDevice->allocateDescriptorSets(allocInfo);
    descriptorSet = std::make_unique<vk::raii::DescriptorSet>(std::move(sets[0]));

    // 5. Update descriptor set
    auto imageInfos = getDescriptorImageInfos(defaultView, defaultSampler);
    auto bufferInfos = getDescriptorBufferInfos();

    std::vector<vk::WriteDescriptorSet> writes;
    for (uint32_t i = 0; i < 5; ++i) {
        writes.push_back({
            .dstSet = **descriptorSet,
            .dstBinding = i,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfos[i]
        });
    }
    writes.push_back({
        .dstSet = **descriptorSet,
        .dstBinding = 5,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &bufferInfos[0]
    });

    logicalDevice->updateDescriptorSets(writes, {});
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
