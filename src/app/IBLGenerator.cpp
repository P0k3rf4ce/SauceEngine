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
    uint32_t faceSize;
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

    // Load HDR image
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

    // Create a descriptor pool for the generation process
    std::array<vk::DescriptorPoolSize, 3> poolSizes = {{
        { vk::DescriptorType::eCombinedImageSampler, 10 },
        { vk::DescriptorType::eStorageImage, 10 },
        { vk::DescriptorType::eSampler, 10 }
    }};
    vk::DescriptorPoolCreateInfo poolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 20,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    descriptorPool = vk::raii::DescriptorPool(*logicalDevice, poolInfo);

    // IBL Sampler
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

    convertEquirectangularToCubemap(hdrView, *maps, commandPool, queue);
    generateIrradianceMap(*maps, commandPool, queue);
    generatePrefilterMap(*maps, commandPool, queue);
    generateBRDFLUT(*maps, commandPool, queue);

    return maps;
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

IBLGenerator::ComputePipeline IBLGenerator::createComputePipeline(
    const std::string& shaderPath, 
    const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
    uint32_t pushConstantSize
) {
    ComputePipeline cp;
    
    vk::DescriptorSetLayoutCreateInfo layoutInfo {
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    cp.layout = vk::raii::DescriptorSetLayout(*logicalDevice, layoutInfo);

    vk::PushConstantRange pushRange {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = pushConstantSize
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &*cp.layout,
        .pushConstantRangeCount = pushConstantSize > 0 ? 1u : 0u,
        .pPushConstantRanges = pushConstantSize > 0 ? &pushRange : nullptr
    };
    cp.pipelineLayout = vk::raii::PipelineLayout(*logicalDevice, pipelineLayoutInfo);

    vk::raii::ShaderModule shaderModule = createShaderModule(shaderPath);
    vk::PipelineShaderStageCreateInfo stageInfo {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = *shaderModule,
        .pName = "computeMain"
    };

    vk::ComputePipelineCreateInfo pipelineInfo {
        .stage = stageInfo,
        .layout = *cp.pipelineLayout
    };
    cp.pipeline = vk::raii::Pipeline(*logicalDevice, nullptr, pipelineInfo);

    return cp;
}

void IBLGenerator::convertEquirectangularToCubemap(const vk::raii::ImageView& hdrView, IBLMaps& maps, const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    uint32_t resolution = 512;
    vk::Format format = vk::Format::eR16G16B16A16Sfloat;
    sauce::ImageUtils::createImage(physicalDevice, logicalDevice, resolution, resolution, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal, maps.envCubemap, maps.envCubemapMemory, 1, 6, vk::ImageCreateFlagBits::eCubeCompatible);
    maps.envCubemapView = sauce::ImageUtils::createImageView(logicalDevice, maps.envCubemap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube, 1, 6);

    vk::raii::ImageView storageView = sauce::ImageUtils::createImageView(logicalDevice, maps.envCubemap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2DArray, 1, 6);

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute },
        { 1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eCompute },
        { 2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute }
    };
    ComputePipeline cp = createComputePipeline("shaders/ibl_equirect_to_cube.spv", bindings, sizeof(uint32_t));

    vk::DescriptorSetAllocateInfo allocInfo { .descriptorPool = descriptorPool, .descriptorSetCount = 1, .pSetLayouts = &*cp.layout };
    vk::raii::DescriptorSet ds = std::move(logicalDevice->allocateDescriptorSets(allocInfo)[0]);

    vk::DescriptorImageInfo hdrInfo { .sampler = *maps.sampler, .imageView = *hdrView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    vk::DescriptorImageInfo samplerInfo { .sampler = *maps.sampler };
    vk::DescriptorImageInfo storageInfo { .imageView = *storageView, .imageLayout = vk::ImageLayout::eGeneral };

    std::array<vk::WriteDescriptorSet, 3> writes;
    writes[0] = { .dstSet = *ds, .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &hdrInfo };
    writes[1] = { .dstSet = *ds, .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eSampler, .pImageInfo = &samplerInfo };
    writes[2] = { .dstSet = *ds, .dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = &storageInfo };
    logicalDevice->updateDescriptorSets(writes, {});

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.envCubemap, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {}, vk::AccessFlagBits2::eShaderStorageWrite, vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eComputeShader, 1, 6);

    vk::raii::CommandBuffer cmd = std::move(vk::raii::CommandBuffers(*logicalDevice, { .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 })[0]);
    cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *cp.pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *cp.pipelineLayout, 0, *ds, nullptr);
    cmd.pushConstants<uint32_t>(*cp.pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, resolution);
    cmd.dispatch((resolution + 7) / 8, (resolution + 7) / 8, 6);
    cmd.end();

    vk::SubmitInfo submitInfo { .commandBufferCount = 1, .pCommandBuffers = &*cmd };
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.envCubemap, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eShaderStorageWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader, vk::PipelineStageFlagBits2::eFragmentShader, 1, 6);
}

void IBLGenerator::generateIrradianceMap(IBLMaps& maps, const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    uint32_t resolution = 32;
    vk::Format format = vk::Format::eR16G16B16A16Sfloat;
    sauce::ImageUtils::createImage(physicalDevice, logicalDevice, resolution, resolution, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal, maps.irradianceMap, maps.irradianceMapMemory, 1, 6, vk::ImageCreateFlagBits::eCubeCompatible);
    maps.irradianceMapView = sauce::ImageUtils::createImageView(logicalDevice, maps.irradianceMap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube, 1, 6);

    vk::raii::ImageView storageView = sauce::ImageUtils::createImageView(logicalDevice, maps.irradianceMap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2DArray, 1, 6);

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute },
        { 1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eCompute },
        { 2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute }
    };
    ComputePipeline cp = createComputePipeline("shaders/ibl_irradiance.spv", bindings, sizeof(uint32_t));

    vk::DescriptorSetAllocateInfo allocInfo { .descriptorPool = descriptorPool, .descriptorSetCount = 1, .pSetLayouts = &*cp.layout };
    vk::raii::DescriptorSet ds = std::move(logicalDevice->allocateDescriptorSets(allocInfo)[0]);

    vk::DescriptorImageInfo envInfo { .sampler = *maps.sampler, .imageView = *maps.envCubemapView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    vk::DescriptorImageInfo samplerInfo { .sampler = *maps.sampler };
    vk::DescriptorImageInfo storageInfo { .imageView = *storageView, .imageLayout = vk::ImageLayout::eGeneral };

    std::array<vk::WriteDescriptorSet, 3> writes;
    writes[0] = { .dstSet = *ds, .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &envInfo };
    writes[1] = { .dstSet = *ds, .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eSampler, .pImageInfo = &samplerInfo };
    writes[2] = { .dstSet = *ds, .dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = &storageInfo };
    logicalDevice->updateDescriptorSets(writes, {});

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.irradianceMap, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {}, vk::AccessFlagBits2::eShaderStorageWrite, vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eComputeShader, 1, 6);

    vk::raii::CommandBuffer cmd = std::move(vk::raii::CommandBuffers(*logicalDevice, { .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 })[0]);
    cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *cp.pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *cp.pipelineLayout, 0, *ds, nullptr);
    cmd.pushConstants<uint32_t>(*cp.pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, resolution);
    cmd.dispatch((resolution + 7) / 8, (resolution + 7) / 8, 6);
    cmd.end();

    vk::SubmitInfo submitInfo { .commandBufferCount = 1, .pCommandBuffers = &*cmd };
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.irradianceMap, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eShaderStorageWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader, vk::PipelineStageFlagBits2::eFragmentShader, 1, 6);
}

void IBLGenerator::generatePrefilterMap(IBLMaps& maps, const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    uint32_t resolution = 128;
    uint32_t mipLevels = 5;
    vk::Format format = vk::Format::eR16G16B16A16Sfloat;
    sauce::ImageUtils::createImage(physicalDevice, logicalDevice, resolution, resolution, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal, maps.prefilterMap, maps.prefilterMapMemory, mipLevels, 6, vk::ImageCreateFlagBits::eCubeCompatible);
    maps.prefilterMapView = sauce::ImageUtils::createImageView(logicalDevice, maps.prefilterMap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube, mipLevels, 6);

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute },
        { 1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eCompute },
        { 2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute }
    };
    ComputePipeline cp = createComputePipeline("shaders/ibl_prefilter.spv", bindings, sizeof(IBLPushConstants));

    vk::DescriptorImageInfo envInfo { .sampler = *maps.sampler, .imageView = *maps.envCubemapView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    vk::DescriptorImageInfo samplerInfo { .sampler = *maps.sampler };

    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
        uint32_t mipRes = resolution >> mip;
        float roughness = (float)mip / (float)(mipLevels - 1);

        vk::raii::ImageView storageView = sauce::ImageUtils::createImageView(logicalDevice, maps.prefilterMap, format, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::e2DArray, 1, 6, mip);

        vk::DescriptorSetAllocateInfo allocInfo { .descriptorPool = descriptorPool, .descriptorSetCount = 1, .pSetLayouts = &*cp.layout };
        vk::raii::DescriptorSet ds = std::move(logicalDevice->allocateDescriptorSets(allocInfo)[0]);

        vk::DescriptorImageInfo storageInfo { .imageView = *storageView, .imageLayout = vk::ImageLayout::eGeneral };

        std::array<vk::WriteDescriptorSet, 3> writes;
        writes[0] = { .dstSet = *ds, .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &envInfo };
        writes[1] = { .dstSet = *ds, .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eSampler, .pImageInfo = &samplerInfo };
        writes[2] = { .dstSet = *ds, .dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = &storageInfo };
        logicalDevice->updateDescriptorSets(writes, {});

        sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.prefilterMap, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {}, vk::AccessFlagBits2::eShaderStorageWrite, vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eComputeShader, 1, 6, mip);

        vk::raii::CommandBuffer cmd = std::move(vk::raii::CommandBuffers(*logicalDevice, { .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 })[0]);
        cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *cp.pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *cp.pipelineLayout, 0, *ds, nullptr);
        IBLPushConstants pc { .faceSize = mipRes, .roughness = roughness };
        cmd.pushConstants<IBLPushConstants>(*cp.pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pc);
        cmd.dispatch((mipRes + 7) / 8, (mipRes + 7) / 8, 6);
        cmd.end();

        vk::SubmitInfo submitInfo { .commandBufferCount = 1, .pCommandBuffers = &*cmd };
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();

        sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.prefilterMap, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eShaderStorageWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader, vk::PipelineStageFlagBits2::eFragmentShader, 1, 6, mip);
    }
}

void IBLGenerator::generateBRDFLUT(IBLMaps& maps, const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue) {
    uint32_t resolution = 512;
    vk::Format format = vk::Format::eR16G16Sfloat;
    sauce::ImageUtils::createImage(physicalDevice, logicalDevice, resolution, resolution, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal, maps.brdfLUT, maps.brdfLUTMemory);
    maps.brdfLUTView = sauce::ImageUtils::createImageView(logicalDevice, maps.brdfLUT, format, vk::ImageAspectFlagBits::eColor);

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute }
    };
    ComputePipeline cp = createComputePipeline("shaders/ibl_brdf_lut.spv", bindings, sizeof(uint32_t) * 2);

    vk::DescriptorSetAllocateInfo allocInfo { .descriptorPool = descriptorPool, .descriptorSetCount = 1, .pSetLayouts = &*cp.layout };
    vk::raii::DescriptorSet ds = std::move(logicalDevice->allocateDescriptorSets(allocInfo)[0]);

    vk::DescriptorImageInfo storageInfo { .imageView = *maps.brdfLUTView, .imageLayout = vk::ImageLayout::eGeneral };

    vk::WriteDescriptorSet write { .dstSet = *ds, .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = &storageInfo };
    logicalDevice->updateDescriptorSets(write, {});

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.brdfLUT, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {}, vk::AccessFlagBits2::eShaderStorageWrite, vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eComputeShader);

    vk::raii::CommandBuffer cmd = std::move(vk::raii::CommandBuffers(*logicalDevice, { .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 })[0]);
    cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *cp.pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *cp.pipelineLayout, 0, *ds, nullptr);
    uint32_t pc[2] = { resolution, resolution };
    cmd.pushConstants<uint32_t>(*cp.pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pc);
    cmd.dispatch((resolution + 15) / 16, (resolution + 15) / 16, 1);
    cmd.end();

    vk::SubmitInfo submitInfo { .commandBufferCount = 1, .pCommandBuffers = &*cmd };
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();

    sauce::ImageUtils::transitionImageLayout(logicalDevice, commandPool, queue, maps.brdfLUT, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eShaderStorageWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader, vk::PipelineStageFlagBits2::eFragmentShader);
}

} // namespace sauce
