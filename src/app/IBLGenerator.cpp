#include <app/IBLGenerator.hpp>
#include <app/ImageUtils.hpp>
#include <app/BufferUtils.hpp>
#include <app/Vertex.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <fstream>
#include <iostream>
#include <array>

namespace sauce {

struct IBLPushConstants {
    glm::mat4 mvp;
    float roughness;
};

IBLGenerator::IBLGenerator(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice)
    : physicalDevice(physicalDevice), logicalDevice(logicalDevice) {
}

std::unique_ptr<IBLMaps> IBLGenerator::generateIBLMaps(
    const std::string& hdrPath,
    const vk::raii::CommandPool& commandPool,
    const vk::raii::Queue& queue
) {
    auto maps = std::make_unique<IBLMaps>();

    // loading HDR image
    int width, height, channels;
    float* pixels = stbi_loadf(hdrPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to load HDR image: " + hdrPath);
    }

    vk::DeviceSize imageSize = width * height * 4 * sizeof(float);
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingMemory = nullptr;
    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice, imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingMemory
    );
    void* data = stagingMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingMemory.unmapMemory();
    stbi_image_free(pixels);

    vk::raii::Image hdrImage = nullptr;
    vk::raii::DeviceMemory hdrImageMemory = nullptr;
    sauce::ImageUtils::createImage(
        physicalDevice, logicalDevice, width, height,
        vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        hdrImage, hdrImageMemory
    );

    sauce::ImageUtils::transitionImageLayout(
        logicalDevice, commandPool, queue, hdrImage,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
        {}, vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eTransfer
    );
    sauce::ImageUtils::copyBufferToImage(logicalDevice, commandPool, queue, stagingBuffer, hdrImage, width, height);
    sauce::ImageUtils::transitionImageLayout(
        logicalDevice, commandPool, queue, hdrImage,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite, vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eFragmentShader
    );

    vk::raii::ImageView hdrView = sauce::ImageUtils::createImageView(
        logicalDevice, hdrImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor
    );

    // map sampler
    vk::SamplerCreateInfo samplerInfo {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .minLod = 0.0f,
        .maxLod = 16.0f,
    };
    maps->sampler = vk::raii::Sampler(*logicalDevice, samplerInfo);

    createCubeMesh(commandPool, queue);

    convertEquirectangularToCubemap(hdrView, *maps, commandPool, queue);
    generateIrradianceMap(*maps, commandPool, queue);
    generatePrefilterMap(*maps, commandPool, queue);
    generateBRDFLUT(*maps, commandPool, queue);

    return maps;
}

void IBLGenerator::createCubeMesh(const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    std::vector<Vertex> cubeVertices = {
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
    };

    std::vector<uint16_t> cubeIndices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 1, 5, 5, 4, 0,
        2, 3, 7, 7, 6, 2,
        0, 3, 7, 7, 4, 0,
        1, 2, 6, 6, 5, 1,
    };
    indexCount = static_cast<uint32_t>(cubeIndices.size());

    vk::DeviceSize vertexBufferSize = cubeVertices.size() * sizeof(Vertex);
    vk::raii::Buffer vertexStagingBuffer = nullptr;
    vk::raii::DeviceMemory vertexStagingMemory = nullptr;
    sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexStagingBuffer, vertexStagingMemory);
    void* vertexData = vertexStagingMemory.mapMemory(0, vertexBufferSize);
    memcpy(vertexData, cubeVertices.data(), vertexBufferSize);
    vertexStagingMemory.unmapMemory();
    sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);
    sauce::BufferUtils::copyBuffer(logicalDevice, commandPool, queue, vertexStagingBuffer, vertexBuffer, vertexBufferSize);

    vk::DeviceSize indexBufferSize = cubeIndices.size() * sizeof(uint16_t);
    vk::raii::Buffer indexStagingBuffer = nullptr;
    vk::raii::DeviceMemory indexStagingMemory = nullptr;
    sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexStagingBuffer, indexStagingMemory);
    void* indexData = indexStagingMemory.mapMemory(0, indexBufferSize);
    memcpy(indexData, cubeIndices.data(), indexBufferSize);
    indexStagingMemory.unmapMemory();
    sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);
    sauce::BufferUtils::copyBuffer(logicalDevice, commandPool, queue, indexStagingBuffer, indexBuffer, indexBufferSize);
}

vk::raii::ShaderModule IBLGenerator::createShaderModule(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open shader file: " + filename);
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    vk::ShaderModuleCreateInfo createInfo{.codeSize = buffer.size() * sizeof(uint32_t), .pCode = buffer.data()};
    return vk::raii::ShaderModule(*logicalDevice, createInfo);
}

void IBLGenerator::convertEquirectangularToCubemap(const vk::raii::ImageView& hdrView, IBLMaps& maps, const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    uint32_t resolution = 512;
    vk::Format format = vk::Format::eR16G16B16A16Sfloat;
    sauce::ImageUtils::createImage(physicalDevice, logicalDevice, resolution, resolution, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, maps.envCubemap, maps.envCubemapMemory, 1, 6, vk::ImageCreateFlagBits::eCubeCompatible);
    maps.envCubemapView = sauce::ImageUtils::createImageView(logicalDevice, maps.envCubemap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube, 1, 6);

    // TODO: rendering logic for 6 faces would go here i think, using dynamic rendering

    // transition to ShaderReadOnlyOptimal for subsequent passes
    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.envCubemap, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, {}, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eFragmentShader, 1, 6);
}

void IBLGenerator::generateIrradianceMap(IBLMaps& maps, const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    uint32_t resolution = 32;
    vk::Format format = vk::Format::eR16G16B16A16Sfloat;
    sauce::ImageUtils::createImage(physicalDevice, logicalDevice, resolution, resolution, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, maps.irradianceMap, maps.irradianceMapMemory, 1, 6, vk::ImageCreateFlagBits::eCubeCompatible);
    maps.irradianceMapView = sauce::ImageUtils::createImageView(logicalDevice, maps.irradianceMap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube, 1, 6);

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.irradianceMap, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, {}, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eFragmentShader, 1, 6);
}

void IBLGenerator::generatePrefilterMap(IBLMaps& maps, const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    uint32_t resolution = 128;
    uint32_t mipLevels = 5;
    vk::Format format = vk::Format::eR16G16B16A16Sfloat;
    sauce::ImageUtils::createImage(physicalDevice, logicalDevice, resolution, resolution, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, maps.prefilterMap, maps.prefilterMapMemory, mipLevels, 6, vk::ImageCreateFlagBits::eCubeCompatible);
    maps.prefilterMapView = sauce::ImageUtils::createImageView(logicalDevice, maps.prefilterMap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube, mipLevels, 6);

    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
        float roughness = (float)mip / (float)(mipLevels - 1);
        for (uint32_t face = 0; face < 6; ++face) {
            // TODO: rendering each face for each mip level with specific roughness
        }
    }

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.prefilterMap, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, {}, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eFragmentShader, mipLevels, 6);
}

void IBLGenerator::generateBRDFLUT(IBLMaps& maps, const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    uint32_t resolution = 512;
    vk::Format format = vk::Format::eR16G16Sfloat;
    sauce::ImageUtils::createImage(physicalDevice, logicalDevice, resolution, resolution, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, maps.brdfLUT, maps.brdfLUTMemory);
    maps.brdfLUTView = sauce::ImageUtils::createImageView(logicalDevice, maps.brdfLUT, format, vk::ImageAspectFlagBits::eColor);

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.brdfLUT, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, {}, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eFragmentShader);
}

} // namespace sauce
