#include "app/components/PointLightComponent.hpp"
#include "app/ImageUtils.hpp"

namespace sauce {

std::unique_ptr<vk::raii::DescriptorSetLayout> PointLightComponent::descriptorSetLayout = nullptr;

PointLightComponent::PointLightComponent()
    : LightComponent(LightType::Point, "PointLightComponent") {}

PointLightComponent::PointLightComponent(const glm::vec3& color, float intensity, float range)
    : LightComponent(LightType::Point, "PointLightComponent")
    , range(range)
{
    this->color = color;
    this->intensity = intensity;
}

GPUPointLight PointLightComponent::toGPUPointLight(const glm::vec3& worldPosition) const {
    GPUPointLight gpu{};
    gpu.position  = worldPosition;
    gpu.color     = color;
    gpu.ambient   = ambient;
    gpu.diffuse   = diffuse;
    gpu.specular  = specular;
    gpu.constant  = constant;
    gpu.linear    = linear;
    gpu.quadratic = quadratic;
    return gpu;
}

GPULight PointLightComponent::toGPULight(const glm::vec3& worldPosition) const {
    GPULight gpu{};
    gpu.position  = worldPosition;
    gpu.type      = static_cast<uint32_t>(LightType::Point);
    gpu.direction = glm::vec3(0.0f);
    gpu.intensity = intensity;
    gpu.color     = color;
    gpu.range     = range;
    gpu.innerConeAngle = 0.0f;
    gpu.outerConeAngle = 0.0f;
    return gpu;
}

void PointLightComponent::initDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice) {
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

const vk::raii::DescriptorSetLayout& PointLightComponent::getDescriptorSetLayout() {
    return *descriptorSetLayout;
}

void PointLightComponent::cleanup() {
    descriptorSetLayout.reset();
}

void PointLightComponent::initDepthMappingResources(
    const sauce::LogicalDevice& logicalDevice,
    const sauce::PhysicalDevice& physicalDevice,
    const vk::raii::CommandPool& commandPool,
    const vk::raii::Queue& queue,
    const vk::raii::DescriptorPool& descriptorPool,
    const vk::raii::DescriptorSetLayout& layout)
{
    if (depthImage) return;

    uint32_t width = 1024;
    uint32_t height = 1024;

    depthImage = std::make_unique<vk::raii::Image>(nullptr);
    depthImageMemory = std::make_unique<vk::raii::DeviceMemory>(nullptr);

    vk::Format depthFormat = vk::Format::eD32Sfloat;

    sauce::ImageUtils::createImage(
        physicalDevice, logicalDevice,
        width, height, depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        *depthImage, *depthImageMemory,
        1, 6, vk::ImageCreateFlagBits::eCubeCompatible
    );

    depthImageView = std::make_unique<vk::raii::ImageView>(
        sauce::ImageUtils::createImageView(
            logicalDevice, *depthImage, depthFormat,
            vk::ImageAspectFlagBits::eDepth,
            vk::ImageViewType::eCube,
            1, 6
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
