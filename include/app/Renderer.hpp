#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/BufferUtils.hpp>
#include <app/Camera.hpp>
#include <app/GraphicsPipeline.hpp>
#include <app/ImageUtils.hpp>
#include <app/LogicalDevice.hpp>
#include <app/Scene.hpp>
#include <app/SwapChain.hpp>

namespace sauce {


const std::vector<Vertex> vertices {
  {{ -0.5f, -0.5f, -0.5f, }, { 0.0f,  0.0f, -1.0f, }, { 0.0f, 0.0f, }, { 1.0f, 0.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, }, 
  {{  0.5f, -0.5f, -0.5f, }, { 0.0f,  0.0f, -1.0f, }, { 0.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, }, 
  {{  0.5f,  0.5f, -0.5f, }, { 0.0f,  0.0f, -1.0f, }, { 0.0f, 0.0f, }, { 0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{ -0.5f,  0.5f, -0.5f, }, { 0.0f,  0.0f, -1.0f, }, { 0.0f, 0.0f, }, { 1.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
                                                                       
  {{ -0.5f, -0.5f,  0.5f, }, { 0.0f,  0.0f,  1.0f, }, { 0.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f, -0.5f,  0.5f, }, { 0.0f,  0.0f,  1.0f, }, { 0.0f, 0.0f, }, { 0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f,  0.5f,  0.5f, }, { 0.0f,  0.0f,  1.0f, }, { 0.0f, 0.0f, }, { 1.0f, 0.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{ -0.5f,  0.5f,  0.5f, }, { 0.0f,  0.0f,  1.0f, }, { 0.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
                                                                       
  {{ -0.5f,  0.5f,  0.5f, }, {-1.0f,  0.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{ -0.5f,  0.5f, -0.5f, }, {-1.0f,  0.0f,  0.0f, }, { 0.0f, 0.0f, }, { 1.0f, 0.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{ -0.5f, -0.5f, -0.5f, }, {-1.0f,  0.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{ -0.5f, -0.5f,  0.5f, }, {-1.0f,  0.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
                                                                       
  {{  0.5f,  0.5f,  0.5f, }, { 1.0f,  0.0f,  0.0f, }, { 0.0f, 0.0f, }, { 1.0f, 0.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f,  0.5f, -0.5f, }, { 1.0f,  0.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f, -0.5f, -0.5f, }, { 1.0f,  0.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f, -0.5f,  0.5f, }, { 1.0f,  0.0f,  0.0f, }, { 0.0f, 0.0f, }, { 1.0f, 0.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
                                                                       
  {{ -0.5f, -0.5f, -0.5f, }, { 0.0f, -1.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f, -0.5f, -0.5f, }, { 0.0f, -1.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f, -0.5f,  0.5f, }, { 0.0f, -1.0f,  0.0f, }, { 0.0f, 0.0f, }, { 1.0f, 0.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{ -0.5f, -0.5f,  0.5f, }, { 0.0f, -1.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
                                                                       
  {{ -0.5f,  0.5f, -0.5f, }, { 0.0f,  1.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f,  0.5f, -0.5f, }, { 0.0f,  1.0f,  0.0f, }, { 0.0f, 0.0f, }, { 1.0f, 0.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{  0.5f,  0.5f,  0.5f, }, { 0.0f,  1.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
  {{ -0.5f,  0.5f,  0.5f, }, { 0.0f,  1.0f,  0.0f, }, { 0.0f, 0.0f, }, { 0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f, 0.0f, 0.0f, }, },
};

const std::vector<uint16_t> indices {
  1, 0, 2, 2, 0, 3,
  4, 5, 6, 6, 7, 4,
  8, 9, 10, 10, 11, 8,
  13, 12, 14, 15, 14, 12,
  16, 17, 18, 18, 19, 16,
  21, 20, 22, 23, 22, 20,
};

struct RendererCreateInfo {
  const sauce::PhysicalDevice& physicalDevice;
  const sauce::LogicalDevice& logicalDevice;
  const sauce::RenderSurface& renderSurface;
  GLFWwindow* window;
};

class Renderer {
public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  Renderer(const RendererCreateInfo& createInfo) {
    queueIndex = createInfo.logicalDevice.getQueueIndex();
    pQueue = std::make_unique<vk::raii::Queue>(*createInfo.logicalDevice, queueIndex, 0);

    pSwapChain = std::make_unique<sauce::SwapChain>(
        createInfo.physicalDevice, 
        createInfo.logicalDevice, 
        createInfo.renderSurface, 
        createInfo.window
    );


    createDescriptorSetLayout(createInfo.logicalDevice);
    
    sauce::GraphicsPipelineConfig mainPipelineConfig {
      .physicalDevice = createInfo.physicalDevice,
      .logicalDevice = createInfo.logicalDevice,
      .descriptorSetLayout = descriptorSetLayout,
      .colorFormat = pSwapChain->getSurfaceFormat().format,
      .shaderPath = "shaders/shader_lights.spv",
    };
    pPipeline = std::make_unique<sauce::GraphicsPipeline>(mainPipelineConfig);

    sauce::GraphicsPipelineConfig postProcessPipelineConfig {
      .physicalDevice = createInfo.physicalDevice,
      .logicalDevice = createInfo.logicalDevice,
      .descriptorSetLayout = postProcessDescriptorSetLayout,
      .colorFormat = pSwapChain->getSurfaceFormat().format,
      .shaderPath = "shaders/post_process.spv",
      .useVertexInput = false,
      .useDepthTest = false,
    };
    pPostProcessPipeline = std::make_unique<sauce::GraphicsPipeline>(postProcessPipelineConfig);

    vk::CommandPoolCreateInfo commandPoolCreateInfo {
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueIndex,
    };

    commandPool = vk::raii::CommandPool { *createInfo.logicalDevice, commandPoolCreateInfo };

    createDepthResources(createInfo.physicalDevice, createInfo.logicalDevice);
    createOffscreenResources(createInfo.physicalDevice, createInfo.logicalDevice);
  
    vk::CommandBufferAllocateInfo allocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    commandBuffers = vk::raii::CommandBuffers(*createInfo.logicalDevice, allocInfo);

    // pCamera = std::make_unique<sauce::Camera>( pSwapChain->getExtent().width, pSwapChain->getExtent().height );

    createUniformBuffers(createInfo.physicalDevice, createInfo.logicalDevice);
    createVertexBuffer(createInfo.physicalDevice, createInfo.logicalDevice);
    createIndexBuffer(createInfo.physicalDevice, createInfo.logicalDevice);

    createDescriptorSets(createInfo.logicalDevice);
    createPostProcessDescriptorSets(createInfo.logicalDevice);

    for (size_t i = 0; i < pSwapChain->getImages().size(); ++i) {
      renderFinishedSemaphores.emplace_back(*createInfo.logicalDevice, vk::SemaphoreCreateInfo{});
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      presentCompleteSemaphores.emplace_back(*createInfo.logicalDevice, vk::SemaphoreCreateInfo{});
      inFlightFences.emplace_back(*createInfo.logicalDevice, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
    }
  }

  void createDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice) {
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
    };
    vk::DescriptorSetLayoutCreateInfo dsLayoutInfo {
      .bindingCount = 1,
      .pBindings = &uboLayoutBinding,
    };

    descriptorSetLayout = vk::raii::DescriptorSetLayout{ *logicalDevice, dsLayoutInfo };

    vk::DescriptorSetLayoutBinding samplerLayoutBinding {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };
    vk::DescriptorSetLayoutCreateInfo ppDsLayoutInfo {
      .bindingCount = 1,
      .pBindings = &samplerLayoutBinding,
    };
    postProcessDescriptorSetLayout = vk::raii::DescriptorSetLayout{ *logicalDevice, ppDsLayoutInfo };
  }

  void createDescriptorSets(const sauce::LogicalDevice& logicalDevice) {
    std::vector<vk::DescriptorPoolSize> poolSizes {
      { .type = vk::DescriptorType::eUniformBuffer, .descriptorCount = sauce::Renderer::MAX_FRAMES_IN_FLIGHT },
      { .type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = sauce::Renderer::MAX_FRAMES_IN_FLIGHT }
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo {
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = sauce::Renderer::MAX_FRAMES_IN_FLIGHT * 2,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data(),
    };

    descriptorPool = vk::raii::DescriptorPool{ *logicalDevice, poolCreateInfo };

    std::vector<vk::DescriptorSetLayout> layouts{ MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout };
    vk::DescriptorSetAllocateInfo dsAllocInfo {
      .descriptorPool = descriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()
    };

    descriptorSets = logicalDevice->allocateDescriptorSets(dsAllocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vk::DescriptorBufferInfo bufferInfo {
        .buffer = uniformBuffers[i],
        .offset = 0,
        .range = sizeof(UniformBufferObject),
      };

      vk::WriteDescriptorSet descriptorWrite {
        .dstSet = descriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &bufferInfo,
      };

      logicalDevice->updateDescriptorSets(descriptorWrite, {});
    }
  }

  void createPostProcessDescriptorSets(const sauce::LogicalDevice& logicalDevice) {
    std::vector<vk::DescriptorSetLayout> layouts{ MAX_FRAMES_IN_FLIGHT, *postProcessDescriptorSetLayout };
    vk::DescriptorSetAllocateInfo dsAllocInfo {
      .descriptorPool = descriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()
    };

    postProcessDescriptorSets = logicalDevice->allocateDescriptorSets(dsAllocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vk::DescriptorImageInfo imageInfo {
        .sampler = *offscreenSampler,
        .imageView = *offscreenImageView,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
      };

      vk::WriteDescriptorSet descriptorWrite {
        .dstSet = postProcessDescriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo,
      };

      logicalDevice->updateDescriptorSets(descriptorWrite, {});
    }
  }


  void createDepthResources(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice) {
    vk::Format depthFormat = GraphicsPipeline::findDepthFormat(physicalDevice);
    ImageUtils::createImage(
        physicalDevice,
        logicalDevice,
        pSwapChain->getExtent().width, 
        pSwapChain->getExtent().height, 
        depthFormat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        depthImage,
        depthImageMemory
    );
    depthImageView = ImageUtils::createImageView(logicalDevice, depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
  }

  void createOffscreenResources(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice) {
    vk::Format colorFormat = pSwapChain->getSurfaceFormat().format;
    ImageUtils::createImage(
        physicalDevice,
        logicalDevice,
        pSwapChain->getExtent().width,
        pSwapChain->getExtent().height,
        colorFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        offscreenImage,
        offscreenImageMemory
    );
    offscreenImageView = ImageUtils::createImageView(logicalDevice, offscreenImage, colorFormat, vk::ImageAspectFlagBits::eColor);

    vk::SamplerCreateInfo samplerInfo {
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eClampToEdge,
      .addressModeV = vk::SamplerAddressMode::eClampToEdge,
      .addressModeW = vk::SamplerAddressMode::eClampToEdge,
      .mipLodBias = 0.0f,
      .anisotropyEnable = vk::False,
      .maxAnisotropy = 1.0f,
      .compareEnable = vk::False,
      .compareOp = vk::CompareOp::eAlways,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = vk::BorderColor::eIntOpaqueBlack,
      .unnormalizedCoordinates = vk::False,
    };
    offscreenSampler = vk::raii::Sampler(*logicalDevice, samplerInfo);
  }

  void createUniformBuffers(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice
      ) {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vk::DeviceSize size = sizeof(UniformBufferObject);
      vk::raii::Buffer buf = nullptr;
      vk::raii::DeviceMemory mem = nullptr;
      sauce::BufferUtils::createBuffer(
          physicalDevice,
          logicalDevice,
          size,
          vk::BufferUsageFlagBits::eUniformBuffer,
          vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
          buf,
          mem
      );

      uniformBuffers.emplace_back(std::move(buf));
      uniformBuffersMemory.emplace_back(std::move(mem));
      uniformBuffersMapped.emplace_back(uniformBuffersMemory[i].mapMemory(0, size));
    }
  }

  void createVertexBuffer(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice
      ) {
    vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    sauce::BufferUtils::createBuffer(
        physicalDevice,
        logicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
        stagingBuffer,
        stagingBufferMemory
    );

    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    sauce::BufferUtils::createBuffer(
        physicalDevice,
        logicalDevice,
        bufferSize, 
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer, 
        vertexBufferMemory
    );

    sauce::BufferUtils::copyBuffer(logicalDevice, commandPool, *pQueue, stagingBuffer, vertexBuffer, bufferSize);
  }

  void createIndexBuffer(
      const sauce::PhysicalDevice& physicalDevice, 
      const sauce::LogicalDevice& logicalDevice
      ) {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    sauce::BufferUtils::createBuffer(
        physicalDevice,
        logicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
        stagingBuffer,
        stagingBufferMemory
    );

    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, indices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    sauce::BufferUtils::createBuffer(
        physicalDevice,
        logicalDevice,
        bufferSize, 
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        indexBuffer, 
        indexBufferMemory
    );

    sauce::BufferUtils::copyBuffer(logicalDevice, commandPool, *pQueue, stagingBuffer, indexBuffer, bufferSize);
  }

  void transitionImageLayout(
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask,
    vk::ImageAspectFlags imageAspectFlags
  ) {
    vk::ImageMemoryBarrier2 barrier {
      .srcStageMask = srcStageMask,
      .srcAccessMask = srcAccessMask,
      .dstStageMask = dstStageMask,
      .dstAccessMask = dstAccessMask,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = image,
      .subresourceRange = {
        .aspectMask = imageAspectFlags,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    };

    vk::DependencyInfo dependencyInfo {
      .dependencyFlags = {},
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &barrier,
    };

    commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
  }

  void recordCommandBuffer(uint32_t imageIndex){
    commandBuffers[frameIndex].begin({});

    // Transition offscreen image for rendering
    transitionImageLayout(
      *offscreenImage,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eColorAttachmentOptimal,
      {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::ImageAspectFlagBits::eColor
    );

    transitionImageLayout(
        *depthImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth
    );

    vk::ClearValue clearColor = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    // Pass 1: Render Scene to Offscreen Image
    vk::RenderingAttachmentInfo colorAttachmentInfo = {
      .imageView = *offscreenImageView,
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearColor,
    };

    vk::RenderingAttachmentInfo depthAttachmentInfo {
      .imageView = depthImageView,
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = clearDepth,
    };

    vk::RenderingInfo renderingInfo {
      .renderArea = { 
        .offset = { 0, 0 }, 
        .extent = pSwapChain->getExtent(),
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentInfo,
      .pDepthAttachment = &depthAttachmentInfo,
    };

    commandBuffers[frameIndex].beginRendering(renderingInfo);

    commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, **pPipeline);
    commandBuffers[frameIndex].bindVertexBuffers(0, *vertexBuffer, {0});
    commandBuffers[frameIndex].bindIndexBuffer( *indexBuffer, 0, vk::IndexType::eUint16 );
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pPipeline->getLayout(), 0, *descriptorSets[frameIndex], nullptr);

    commandBuffers[frameIndex].setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(pSwapChain->getExtent().width), 
        static_cast<float>(pSwapChain->getExtent().height), 0.0f, 1.0f));
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), pSwapChain->getExtent()));

    commandBuffers[frameIndex].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    commandBuffers[frameIndex].endRendering();

    // Transition offscreen image for sampling
    transitionImageLayout(
      *offscreenImage,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::AccessFlagBits2::eShaderRead,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eFragmentShader,
      vk::ImageAspectFlagBits::eColor
    );

    // Transition swapchain image for rendering
    transitionImageLayout(
      pSwapChain->getImages()[imageIndex],
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eColorAttachmentOptimal,
      {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::ImageAspectFlagBits::eColor
    );

    // Pass 2: Render Post-processing to Swapchain Image
    vk::RenderingAttachmentInfo ppColorAttachmentInfo = {
      .imageView = pSwapChain->getImageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearColor,
    };

    vk::RenderingInfo ppRenderingInfo {
      .renderArea = { 
        .offset = { 0, 0 }, 
        .extent = pSwapChain->getExtent(),
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &ppColorAttachmentInfo,
    };

    commandBuffers[frameIndex].beginRendering(ppRenderingInfo);

    commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, **pPostProcessPipeline);
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pPostProcessPipeline->getLayout(), 0, *postProcessDescriptorSets[frameIndex], nullptr);

    commandBuffers[frameIndex].setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(pSwapChain->getExtent().width), 
        static_cast<float>(pSwapChain->getExtent().height), 0.0f, 1.0f));
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), pSwapChain->getExtent()));

    // Draw fullscreen quad (3 vertices)
    commandBuffers[frameIndex].draw(3, 1, 0, 0);

    commandBuffers[frameIndex].endRendering();

    transitionImageLayout(
      pSwapChain->getImages()[imageIndex],
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eBottomOfPipe,
      vk::ImageAspectFlagBits::eColor
    );

    commandBuffers[frameIndex].end();
  }

  /**
   * @brief Orchestrates the rendering of a single frame.
   * * This function handles the synchronization between the CPU and GPU by:
   * 1. Waiting for the previous frame's fence to ensure resources are free.
   *    - Fence is a synchronization primitive used to sync the GPU with the CPU.
   * 2. Acquiring an image from the swapchain.
   * 3. Recording and submitting command buffers to the graphics queue.
   * 4. Presenting the finished image back to the screen.
   * * @param logicalDevice The device handle used to manage synchronization primitives.
   * @throws std::runtime_error If synchronization fails or the swapchain becomes invalid.
   */
  void drawFrame(const sauce::LogicalDevice& logicalDevice, const sauce::Scene& scene){
    // Wait for the in-flight fence to be signaled, ensuring the previous frame finished rendering
    // This ensures the CPU doesn't submit additional frames while our previous frames are still rendering
    auto fenceResult = logicalDevice->waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to wait for fence!");
    }

    // Request the next available image from the swap chain (our display engine)
    auto [result, imageIndex] = (*pSwapChain)->acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

    // Verify the swap chain image was acquired successfully (suboptimal is acceptable)
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("Failed to acquire swap chain image");
    }

    // Reset the fence for the next frame
    logicalDevice->resetFences(*inFlightFences[frameIndex]);

    // Reset and record the command buffer with rendering commands
    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);

    // Update the uniform buffer with current transformation matrices
    updateUniformBuffer(frameIndex, scene);

    // Prepare submission: wait for image to be available before starting color attachment output
    vk::PipelineStageFlags waitDestinationStageMask { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    const vk::SubmitInfo submitInfo {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
      .pWaitDstStageMask = &waitDestinationStageMask,
      .commandBufferCount = 1,
      .pCommandBuffers = &*commandBuffers[frameIndex],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex],
    };

    // Submit the command buffer to the queue for execution, signaling the fence when complete
    pQueue->submit(submitInfo, *inFlightFences[frameIndex]);

    // Prepare presentation: wait for rendering to finish before presenting
    const vk::PresentInfoKHR presentInfoKHR {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
      .swapchainCount = 1,
      .pSwapchains = &***pSwapChain,
      .pImageIndices = &imageIndex,
    };

    // Present the rendered image to the screen
    result = pQueue->presentKHR(presentInfoKHR);
    if (result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to present swap chain image!");
    }

    // Advance to the next frame in the circular buffer
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  /**
   * @brief Updates the Uniform Buffer Object (UBO) with current frame transformations.
   * * Calculates the elapsed time to handle object rotation and constructs the 
   *   Model-View-Projection (MVP) matrices. The resulting data is copied directly 
   *   into the GPU-mapped memory for the specific frame being rendered.
   * * @note Vulkan's clip space Y-axis is inverted compared to OpenGL. This function 
   *   manually compensates by flipping the Y-coordinate in the projection matrix.
   * * @param curImage The index of the current swapchain image/frame being processed.
   */
  void updateUniformBuffer(uint32_t curImage, const sauce::Scene& scene) {
    // Record the start time on first call (static initialization)
    static auto startTime = std::chrono::high_resolution_clock::now();

    // Get the current time and calculate elapsed time in seconds
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    // Create uniform buffer object with transformation matrices
    sauce::UniformBufferObject ubo {
      // Model matrix: rotates the object 90 degrees per second around the Z axis
      .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
      // .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.1f)),
      // .proj = glm::perspective(glm::radians(45.0f), static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.1f, 10.0f),
      .view = scene.getCameraRO().getViewMatrix(),
      .proj = scene.getCameraRO().getProjectionMatrix(),
      .cameraPos = scene.getCameraRO().getPos(),
    };
    
    // Flip Y coordinate of projection matrix (Vulkan uses inverted Y compared to OpenGL)
    ubo.proj[1][1] *= -1;

    // Copy the uniform buffer data to GPU-mapped memory for the current frame
    memcpy(uniformBuffersMapped[curImage], &ubo, sizeof(ubo));
  }


private:
  std::unique_ptr<vk::raii::Queue> pQueue;
  std::unique_ptr<sauce::SwapChain> pSwapChain;

  vk::raii::CommandPool commandPool = nullptr;
  
  std::vector<vk::raii::CommandBuffer> commandBuffers;

  std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  std::vector<vk::raii::Fence> inFlightFences;

  uint32_t frameIndex = 0;
  uint32_t queueIndex = ~0;

  std::unique_ptr<sauce::GraphicsPipeline> pPipeline;
  std::unique_ptr<sauce::GraphicsPipeline> pPostProcessPipeline;

  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::DescriptorSetLayout postProcessDescriptorSetLayout = nullptr;
  vk::raii::DescriptorPool descriptorPool = nullptr;
  std::vector<vk::raii::DescriptorSet> descriptorSets;
  std::vector<vk::raii::DescriptorSet> postProcessDescriptorSets;


  vk::raii::Buffer vertexBuffer = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory = nullptr;
  vk::raii::Buffer indexBuffer = nullptr;
  vk::raii::DeviceMemory indexBufferMemory = nullptr;

  std::vector<vk::raii::Buffer> uniformBuffers;
  std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
  std::vector<void *> uniformBuffersMapped;


  vk::raii::Image depthImage = nullptr;
  vk::raii::DeviceMemory depthImageMemory = nullptr;
  vk::raii::ImageView depthImageView = nullptr;

  vk::raii::Image offscreenImage = nullptr;
  vk::raii::DeviceMemory offscreenImageMemory = nullptr;
  vk::raii::ImageView offscreenImageView = nullptr;
  vk::raii::Sampler offscreenSampler = nullptr;
};

}

