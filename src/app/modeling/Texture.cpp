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
    , hdr(false)
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
    , hdr(false)
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

const std::vector<float>& Texture::getHdrData() {
    if (!dataLoaded) {
        loadFromFile();
    }
    return hdrData;
}

void Texture::loadFromFile() {
    if (dataLoaded) {
        return;
    }

    if (embedded) {
        return;
    }

    if (stbi_is_hdr(path.c_str())) {
        loadFromFileHDR();
        return;
    }

    int w, h, c;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &c, STBI_rgb_alpha);

    if (!pixels) {
        createDefaultPixels();
        return;
    }

    width = w;
    height = h;
    channels = 4;

    size_t dataSize = width * height * channels;
    data.resize(dataSize);
    std::memcpy(data.data(), pixels, dataSize);

    stbi_image_free(pixels);
    dataLoaded = true;
}

void Texture::loadFromFileHDR() {
    int w, h, c;
    float* pixels = stbi_loadf(path.c_str(), &w, &h, &c, STBI_rgb_alpha);

    if (!pixels) {
        createDefaultPixels();
        return;
    }

    width = w;
    height = h;
    channels = 4;
    hdr = true;

    size_t pixelCount = static_cast<size_t>(width) * height * channels;
    hdrData.resize(pixelCount);
    std::memcpy(hdrData.data(), pixels, pixelCount * sizeof(float));

    stbi_image_free(pixels);
    dataLoaded = true;
}

void Texture::createDefaultPixels() {
    width = 1;
    height = 1;
    channels = 4;
    data.resize(4);

    if (type == TextureType::Normal) {
        data[0] = 128; data[1] = 128; data[2] = 255; data[3] = 255;
    } else if (type == TextureType::MetallicRoughness || type == TextureType::Occlusion) {
        data[0] = 0; data[1] = 0; data[2] = 0; data[3] = 255;
    } else {
        data[0] = 255; data[1] = 255; data[2] = 255; data[3] = 255;
    }

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

    bool hasPixels = hdr ? !hdrData.empty() : !data.empty();
    if (!hasPixels || width == 0 || height == 0) {
        throw std::runtime_error("No data for " + path);
    }

    vk::DeviceSize bytesPerPixel = hdr ? (4 * sizeof(float)) : 4;
    vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(width) * height * bytesPerPixel;

    vk::raii::Buffer stagingBuffer(nullptr);
    vk::raii::DeviceMemory stagingMemory(nullptr);

    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice, imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingMemory
    );

    void* mappedData = stagingMemory.mapMemory(0, imageSize);
    if (hdr) {
        memcpy(mappedData, hdrData.data(), static_cast<size_t>(imageSize));
    } else {
        memcpy(mappedData, data.data(), static_cast<size_t>(imageSize));
    }
    stagingMemory.unmapMemory();

    vk::Format imageFormat;
    if (hdr) {
        imageFormat = vk::Format::eR32G32B32A32Sfloat;
    } else {
        imageFormat = sRGB ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
    }

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

    bool isEnvMap = (type == TextureType::EnvironmentMapHDR);
    vk::SamplerAddressMode addressMode = isEnvMap
        ? vk::SamplerAddressMode::eClampToEdge
        : vk::SamplerAddressMode::eRepeat;

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.borderColor = hdr
        ? vk::BorderColor::eFloatOpaqueBlack
        : vk::BorderColor::eIntOpaqueBlack;
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
