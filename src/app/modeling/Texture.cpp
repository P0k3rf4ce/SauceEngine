#include "app/modeling/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <app/PhysicalDevice.hpp>
#include <app/LogicalDevice.hpp>
#include <app/BufferUtils.hpp>
#include <app/ImageUtils.hpp>

namespace sauce {
namespace modeling {

Texture::Texture(const std::string& path, TextureType type, bool sRGB)
    : path(path)
    , type(type)
    , sRGB(sRGB)
    , embedded(false)
    , width(0)
    , height(0)
    , channels(0)
    , dataLoaded(false) {
}

Texture::Texture(const std::vector<unsigned char>& data, int width, int height, int channels,
                 TextureType type, bool sRGB, const std::string& name)
    : path(name)
    , type(type)
    , sRGB(sRGB)
    , embedded(true)
    , width(width)
    , height(height)
    , channels(channels)
    , data(data)
    , dataLoaded(true) {
}

const std::vector<unsigned char>& Texture::getData() {
    if (!dataLoaded) {
        loadFromFile();
    }
    return data;
}

void Texture::loadFromFile() {
    if (dataLoaded) {
        return;
    }

    if (embedded) {
        return;
    }

    int w, h, c;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &c, STBI_rgb_alpha);

    if (!pixels) {
        // Create a default 1x1 texture based on type
        width = 1;
        height = 1;
        channels = 4;
        data.resize(4);

        if (type == TextureType::Normal) {
            // Flat normal pointing up (128, 128, 255, 255) -> (0, 0, 1) in tangent space
            data[0] = 128;
            data[1] = 128;
            data[2] = 255;
            data[3] = 255;
        } else if (type == TextureType::MetallicRoughness || type == TextureType::Occlusion) {
            // Black (non-metallic, smooth, no occlusion)
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = 255;
        } else {
            // White default for BaseColor and Emissive
            data[0] = 255;
            data[1] = 255;
            data[2] = 255;
            data[3] = 255;
        }

        dataLoaded = true;
        return;
    }

    width = w;
    height = h;
    channels = 4; // stbi_load with STBI_rgb_alpha always returns 4 channels

    // Copy pixel data
    size_t dataSize = width * height * channels;
    data.resize(dataSize);
    std::memcpy(data.data(), pixels, dataSize);

    stbi_image_free(pixels);
    dataLoaded = true;
}

void Texture::initVulkanResources(
    const sauce::LogicalDevice& logicalDevice,
    vk::raii::PhysicalDevice& physicalDevice,
    vk::raii::CommandPool& commandPool,
    vk::raii::Queue& queue
) {
    if (hasGPUData()) {
        return;
    }

    if (!dataLoaded) {
        getData();
    }

    if (data.empty() || width == 0 || height == 0) {
        throw std::runtime_error("No data for " + path);
    }

    vk::DeviceSize imageSize = (vk::DeviceSize)width * height * 4;

    vk::raii::Buffer stagingBuffer(nullptr);
    vk::raii::DeviceMemory stagingMemory(nullptr);

    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice, imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingMemory
    );

    void* mappedData = stagingMemory.mapMemory(0, imageSize);
    memcpy(mappedData, this->data.data(), (size_t)imageSize);
    stagingMemory.unmapMemory();

    vk::Format imageFormat = sRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

    image = std::make_unique<vk::raii::Image>(nullptr);
    imageMemory = std::make_unique<vk::raii::DeviceMemory>(nullptr);

    sauce::ImageUtils::createImage(
        physicalDevice, logicalDevice,
        width, height, imageFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        *image, *imageMemory
    );

    sauce::ImageUtils::transitionImageLayout(
        logicalDevice, commandPool, queue, *image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        {},
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eTransfer
    );

    sauce::ImageUtils::copyBufferToImage(
        logicalDevice, commandPool, queue,
        stagingBuffer, *image, (uint32_t)width, (uint32_t)height
    );

    sauce::ImageUtils::transitionImageLayout(
        logicalDevice, commandPool, queue, *image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite,
        vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::PipelineStageFlagBits2::eFragmentShader
    );

    imageView = std::make_unique<vk::raii::ImageView>(
        sauce::ImageUtils::createImageView(
            logicalDevice, *image, imageFormat,
            vk::ImageAspectFlagBits::eColor
        )
    );

    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    sampler = std::make_unique<vk::raii::Sampler>(*logicalDevice, samplerInfo);
}

vk::DescriptorImageInfo Texture::getDescriptorInfo() const {
    if (!hasGPUData()) {
        throw std::runtime_error(
            "GPU resources not initialized for " + path
        );
    }

    vk::DescriptorImageInfo info{};
    info.sampler = **sampler;
    info.imageView = **imageView;
    info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    return info;
}

} // namespace modeling
} // namespace sauce
