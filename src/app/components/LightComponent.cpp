#include "app/components/LightComponent.hpp"
#include "app/ImageUtils.hpp"
#include "app/LogicalDevice.hpp"
#include "app/PhysicalDevice.hpp"

namespace sauce {

std::unique_ptr<vk::raii::DescriptorSetLayout> LightComponent::descriptorSetLayout = nullptr;

void LightComponent::initDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice) {
    if (descriptorSetLayout) return;

    vk::DescriptorSetLayoutBinding shadowBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo {
        .bindingCount = 1,
        .pBindings = &shadowBinding,
    };
    descriptorSetLayout = std::make_unique<vk::raii::DescriptorSetLayout>(*logicalDevice, layoutInfo);
}

const vk::raii::DescriptorSetLayout& LightComponent::getDescriptorSetLayout() {
    return *descriptorSetLayout;
}

void LightComponent::cleanup() {
    descriptorSetLayout.reset();
}

void LightComponent::allocateDepthMappingResources(
    const sauce::LogicalDevice& logicalDevice,
    const sauce::PhysicalDevice& physicalDevice,
    const vk::raii::DescriptorPool& descriptorPool,
    uint32_t resolution,
    bool isCubeMap) 
{
    if (depthImage) return;

    depthImage = std::make_unique<vk::raii::Image>(nullptr);
    depthImageMemory = std::make_unique<vk::raii::DeviceMemory>(nullptr);

    vk::Format depthFormat = vk::Format::eD32Sfloat;
    uint32_t layers = isCubeMap ? 6 : 1;
    vk::ImageCreateFlags createFlags = isCubeMap ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags{};

    sauce::ImageUtils::createImage(
        physicalDevice, logicalDevice,
        resolution, resolution, depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        *depthImage, *depthImageMemory,
        1, layers, createFlags
    );

    vk::ImageViewType viewType = isCubeMap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;

    depthImageView = std::make_unique<vk::raii::ImageView>(
        sauce::ImageUtils::createImageView(
            logicalDevice, *depthImage, depthFormat,
            vk::ImageAspectFlagBits::eDepth,
            viewType, 1, layers
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
        .pSetLayouts = &**descriptorSetLayout, // Grab the unified layout
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
}