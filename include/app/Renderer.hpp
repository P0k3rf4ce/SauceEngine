#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/BufferUtils.hpp>
#include <app/Camera.hpp>
#include <app/GraphicsPipeline.hpp>
#include <app/ImGuiRenderer.hpp>
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

struct MaterialData {
  glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f}; // offset  0
  float     metallicFactor{1.0f};                      // offset 16
  float     roughnessFactor{1.0f};                     // offset 20
  float     normalScale{1.0f};                         // offset 24
  float     occlusionStrength{1.0f};                   // offset 28
  alignas(16) glm::vec3 emissiveFactor{0.0f};          // offset 32
  float     _pad0{0.0f};                               // offset 44
};

struct RendererCreateInfo {
  const sauce::PhysicalDevice& physicalDevice;
  const sauce::LogicalDevice& logicalDevice;
  const sauce::RenderSurface& renderSurface;
  GLFWwindow* window;
  bool vsync = true;
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
        createInfo.window,
        createInfo.vsync
    );


    createDescriptorSetLayout(createInfo.logicalDevice);
    pPipeline = std::make_unique<sauce::GraphicsPipeline>(createInfo.physicalDevice, createInfo.logicalDevice, descriptorSetLayout, *pSwapChain);

    vk::CommandPoolCreateInfo commandPoolCreateInfo {
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueIndex,
    };

    commandPool = vk::raii::CommandPool { *createInfo.logicalDevice, commandPoolCreateInfo };

    createDepthResources(createInfo.physicalDevice, createInfo.logicalDevice);
    createDefaultTextures(createInfo.physicalDevice, createInfo.logicalDevice);
    createMaterialBuffer(createInfo.physicalDevice, createInfo.logicalDevice);
    createLightSSBO(createInfo.physicalDevice, createInfo.logicalDevice);
  
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

    for (size_t i = 0; i < pSwapChain->getImages().size(); ++i) {
      renderFinishedSemaphores.emplace_back(*createInfo.logicalDevice, vk::SemaphoreCreateInfo{});
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      presentCompleteSemaphores.emplace_back(*createInfo.logicalDevice, vk::SemaphoreCreateInfo{});
      inFlightFences.emplace_back(*createInfo.logicalDevice, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
    }
  }

  const vk::raii::Queue& getQueue() const { return *pQueue; }
  const sauce::SwapChain& getSwapChain() const { return *pSwapChain; }
  const vk::raii::CommandPool& getCommandPool() const { return commandPool; }

  void createDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice) {
    std::array<vk::DescriptorSetLayoutBinding, 13> bindings;

    // UBO
    bindings[0] = { .binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer,  .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment };
    // MaterialData
    bindings[1] = { .binding = 1, .descriptorType = vk::DescriptorType::eUniformBuffer,  .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment };
    // Light SSBO
    bindings[2] = { .binding = 2, .descriptorType = vk::DescriptorType::eStorageBuffer,  .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment };
    // Texture and Sampler pairs
    for (uint32_t i = 0; i < 5; ++i) {
      bindings[3 + i * 2] = { .binding = 3 + i * 2, .descriptorType = vk::DescriptorType::eSampledImage, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment };
      bindings[4 + i * 2] = { .binding = 4 + i * 2, .descriptorType = vk::DescriptorType::eSampler,      .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment };
    }

    vk::DescriptorSetLayoutCreateInfo dsLayoutInfo {
      .bindingCount = static_cast<uint32_t>(bindings.size()),
      .pBindings = bindings.data(),
    };

    descriptorSetLayout = vk::raii::DescriptorSetLayout{ *logicalDevice, dsLayoutInfo };
  }

  void createDescriptorSets(const sauce::LogicalDevice& logicalDevice) {
    std::array<vk::DescriptorPoolSize, 4> poolSizes {{
      { vk::DescriptorType::eUniformBuffer, 2u * MAX_FRAMES_IN_FLIGHT },
      { vk::DescriptorType::eStorageBuffer, 1u * MAX_FRAMES_IN_FLIGHT },
      { vk::DescriptorType::eSampledImage,  5u * MAX_FRAMES_IN_FLIGHT },
      { vk::DescriptorType::eSampler,       5u * MAX_FRAMES_IN_FLIGHT },
    }};

    vk::DescriptorPoolCreateInfo poolCreateInfo {
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
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

    vk::DescriptorBufferInfo materialInfo { .buffer = *materialBuffer, .offset = 0, .range = sizeof(MaterialData) };
    vk::DescriptorBufferInfo lightSSBOInfo { .buffer = *lightSSBO,     .offset = 0, .range = lightSSBOSize };
    vk::DescriptorImageInfo  imageInfo     { .imageView = *defaultImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    vk::DescriptorImageInfo  samplerInfo   { .sampler = *defaultSampler };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vk::DescriptorBufferInfo uboInfo { .buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject) };

      std::array<vk::WriteDescriptorSet, 13> writes;
      writes[0] = { .dstSet = descriptorSets[i], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer,  .pBufferInfo = &uboInfo };
      writes[1] = { .dstSet = descriptorSets[i], .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer,  .pBufferInfo = &materialInfo };
      writes[2] = { .dstSet = descriptorSets[i], .dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer,  .pBufferInfo = &lightSSBOInfo };
      for (uint32_t t = 0; t < 5; ++t) {
        writes[3 + t * 2] = { .dstSet = descriptorSets[i], .dstBinding = 3 + t * 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eSampledImage, .pImageInfo = &imageInfo };
        writes[4 + t * 2] = { .dstSet = descriptorSets[i], .dstBinding = 4 + t * 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eSampler,      .pImageInfo = &samplerInfo };
      }

      logicalDevice->updateDescriptorSets(writes, {});
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

  void recordCommandBuffer(uint32_t imageIndex, sauce::ImGuiRenderer* imguiRenderer){
    commandBuffers[frameIndex].begin({});

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

    vk::RenderingAttachmentInfo colorAttachmentInfo = {
      .imageView = pSwapChain->getImageViews()[imageIndex],
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

    const uint32_t lightCount = 0;
    commandBuffers[frameIndex].pushConstants<uint32_t>(
        *pPipeline->getLayout(),
        vk::ShaderStageFlagBits::eFragment,
        0u, { lightCount }
    );

    commandBuffers[frameIndex].drawIndexed(indices.size(), 1, 0, 0, 0);

    // Render ImGui overlay
    if (imguiRenderer) {
      imguiRenderer->render(commandBuffers[frameIndex], imageIndex);
    }

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
  void drawFrame(const sauce::LogicalDevice& logicalDevice, const sauce::Scene& scene, sauce::ImGuiRenderer* imguiRenderer = nullptr){
    // Wait for the previous frame to finish rendering before submitting the next frame
    auto fenceResult = logicalDevice->waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to wait for fence!");
    }

    // Request the next available image from the swap chain
    auto [result, imageIndex] = (*pSwapChain)->acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

    // Verify the swap chain image was acquired successfully (suboptimal is acceptable)
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("Failed to acquire swap chain image");
    }

    // Reset the fence for the next frame
    logicalDevice->resetFences(*inFlightFences[frameIndex]);

    // Reset and record the command buffer with rendering commands
    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex, imguiRenderer);

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


  // Creates a 1x1 white pixel image + sampler used as fallbacks for all 5 PBR texture slots.
  void createDefaultTextures(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice) {
    uint8_t whitePixel[4] = {255, 255, 255, 255};
    vk::DeviceSize pixelSize = sizeof(whitePixel);

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingMemory = nullptr;
    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice, pixelSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingMemory
    );
    void* mapped = stagingMemory.mapMemory(0, pixelSize);
    memcpy(mapped, whitePixel, pixelSize);
    stagingMemory.unmapMemory();

    ImageUtils::createImage(
        physicalDevice, logicalDevice, 1, 1,
        vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        defaultImage, defaultImageMemory
    );

    ImageUtils::transitionImageLayout(
        logicalDevice, commandPool, *pQueue, defaultImage,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
        {}, vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eTransfer
    );
    ImageUtils::copyBufferToImage(logicalDevice, commandPool, *pQueue, stagingBuffer, defaultImage, 1, 1);
    ImageUtils::transitionImageLayout(
        logicalDevice, commandPool, *pQueue, defaultImage,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite, vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eFragmentShader
    );

    defaultImageView = ImageUtils::createImageView(logicalDevice, defaultImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

    vk::SamplerCreateInfo samplerInfo {
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eRepeat,
      .addressModeV = vk::SamplerAddressMode::eRepeat,
      .addressModeW = vk::SamplerAddressMode::eRepeat,
      .minLod = 0.0f,
      .maxLod = 0.0f,
    };
    defaultSampler = vk::raii::Sampler{ *logicalDevice, samplerInfo };
  }

  // Creates a host-visible uniform buffer holding default PBR material properties.
  void createMaterialBuffer(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice) {
    MaterialData defaults{};
    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice, sizeof(MaterialData),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        materialBuffer, materialBufferMemory
    );
    void* data = materialBufferMemory.mapMemory(0, sizeof(MaterialData));
    memcpy(data, &defaults, sizeof(MaterialData));
    materialBufferMemory.unmapMemory();
  }

  // Allocates a storage buffer for lights. lightCount = 0 for now; one slot pre-allocated
  // because Vulkan does not permit zero-size buffers.
  void createLightSSBO(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice) {
    lightSSBOSize = 64; // sizeof one Light struct (matches shader layout)
    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice, lightSSBOSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        lightSSBO, lightSSBOMemory
    );
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

  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::DescriptorPool descriptorPool = nullptr;
  std::vector<vk::raii::DescriptorSet> descriptorSets;


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

  // PBR resources
  vk::raii::Buffer materialBuffer = nullptr;
  vk::raii::DeviceMemory materialBufferMemory = nullptr;

  vk::DeviceSize lightSSBOSize{0};
  vk::raii::Buffer lightSSBO = nullptr;
  vk::raii::DeviceMemory lightSSBOMemory = nullptr;

  vk::raii::Image defaultImage = nullptr;
  vk::raii::DeviceMemory defaultImageMemory = nullptr;
  vk::raii::ImageView defaultImageView = nullptr;
  vk::raii::Sampler defaultSampler = nullptr;
};

}

