#include "app/components/DirectionalLightComponent.hpp"
#include "app/ImageUtils.hpp"

namespace sauce {

std::unique_ptr<vk::raii::DescriptorSetLayout> DirectionalLightComponent::descriptorSetLayout = nullptr;

DirectionalLightComponent::DirectionalLightComponent()
    : LightComponent(LightType::Directional, "DirectionalLightComponent") {}

DirectionalLightComponent::DirectionalLightComponent(const glm::vec3& color, float intensity)
    : LightComponent(LightType::Directional, "DirectionalLightComponent")
{
    this->color = color;
    this->intensity = intensity;
}

GPULight DirectionalLightComponent::toGPULight(const glm::vec3& worldPosition) const {
    GPULight gpu{};
    gpu.position  = worldPosition;
    gpu.type      = static_cast<uint32_t>(LightType::Directional);
    gpu.direction = glm::vec3(0.0f, 0.0f, -1.0f); // Default direction, will be overridden by Scene
    gpu.intensity = intensity;
    gpu.color     = color;
    gpu.range     = 0.0f;
    gpu.innerConeAngle = 0.0f;
    gpu.outerConeAngle = 0.0f;
    return gpu;
}

void DirectionalLightComponent::initDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice) {
    if (descriptorSetLayout) return;

    vk::DescriptorSetLayoutBinding shadowBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo {
        .bindingCount = 1,
        .pBindings = &shadowBinding,
    };
    descriptorSetLayout = std::make_unique<vk::raii::DescriptorSetLayout>(*logicalDevice, layoutInfo);
}

const vk::raii::DescriptorSetLayout& DirectionalLightComponent::getDescriptorSetLayout() {
    return *descriptorSetLayout;
}

void DirectionalLightComponent::cleanup() {
    descriptorSetLayout.reset();
}

void DirectionalLightComponent::initDepthMappingResources(
    const sauce::LogicalDevice& logicalDevice,
    const sauce::PhysicalDevice& physicalDevice,
    const vk::raii::CommandPool& commandPool,
    const vk::raii::Queue& queue,
    const vk::raii::DescriptorPool& descriptorPool,
    const vk::raii::DescriptorSetLayout& layout)
{
    if (depthImage) return;

    uint32_t width = 2048;
    uint32_t height = 2048;

    depthImage = std::make_unique<vk::raii::Image>(nullptr);
    depthImageMemory = std::make_unique<vk::raii::DeviceMemory>(nullptr);

    vk::Format depthFormat = vk::Format::eD32Sfloat;

    sauce::ImageUtils::createImage(
        physicalDevice, logicalDevice,
        width, height, depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        *depthImage, *depthImageMemory
    );

    depthImageView = std::make_unique<vk::raii::ImageView>(
        sauce::ImageUtils::createImageView(
            logicalDevice, *depthImage, depthFormat,
            vk::ImageAspectFlagBits::eDepth
        )
    );

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = vk::CompareOp::eLessOrEqual;
    samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;

    depthSampler = std::make_unique<vk::raii::Sampler>(*logicalDevice, samplerInfo);

    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*layout,
    };

    auto sets = logicalDevice->allocateDescriptorSets(allocInfo);
    depthDescriptorSet = std::make_unique<vk::raii::DescriptorSet>(std::move(sets[0]));

    vk::DescriptorImageInfo imageInfo {
        .sampler = **depthSampler,
        .imageView = **depthImageView,
        .imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal
    };

    vk::WriteDescriptorSet write {
        .dstSet = **depthDescriptorSet,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo
    };

    logicalDevice->updateDescriptorSets(write, {});
}

} // namespace sauce
