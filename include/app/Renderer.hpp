#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <chrono>
#include <functional>
#include <iostream>

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

struct RendererCreateInfo {
  const sauce::PhysicalDevice& physicalDevice;
  const sauce::LogicalDevice& logicalDevice;
  const sauce::RenderSurface& renderSurface;
  GLFWwindow* window;
};

// Callback type for custom command buffer recording
using CommandBufferRecorder = std::function<void(vk::raii::CommandBuffer&, uint32_t)>;

class Renderer {
public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  Renderer(const RendererCreateInfo& createInfo)
    : pPhysicalDevice(&createInfo.physicalDevice),
      pLogicalDevice(&createInfo.logicalDevice),
      pRenderSurface(&createInfo.renderSurface),
      pWindow(createInfo.window)
  {
    queueIndex = createInfo.logicalDevice.getQueueIndex();
    pQueue = std::make_unique<vk::raii::Queue>(*createInfo.logicalDevice, queueIndex, 0);

    pSwapChain = std::make_unique<sauce::SwapChain>(
        createInfo.physicalDevice,
        createInfo.logicalDevice,
        createInfo.renderSurface,
        createInfo.window
    );


    createDescriptorSetLayout(createInfo.logicalDevice);
    pPipeline = std::make_unique<sauce::GraphicsPipeline>(createInfo.physicalDevice, createInfo.logicalDevice, descriptorSetLayout, *pSwapChain);

    vk::CommandPoolCreateInfo commandPoolCreateInfo {
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueIndex,
    };

    commandPool = vk::raii::CommandPool { *createInfo.logicalDevice, commandPoolCreateInfo };

    createDepthResources(createInfo.physicalDevice, createInfo.logicalDevice);

    vk::CommandBufferAllocateInfo allocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    commandBuffers = vk::raii::CommandBuffers(*createInfo.logicalDevice, allocInfo);

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
  const vk::raii::DescriptorSetLayout& getDescriptorSetLayout() const { return descriptorSetLayout; }
  uint32_t getFrameIndex() const { return frameIndex; }
  const vk::raii::DescriptorSet& getCurrentDescriptorSet() const { return descriptorSets[frameIndex]; }
  void* getCurrentUniformBufferMapped() const { return uniformBuffersMapped[frameIndex]; }

  void setFramebufferResized() { framebufferResized = true; }

  const vk::raii::Image& getDepthImage() const { return depthImage; }
  const vk::raii::ImageView& getDepthImageView() const { return depthImageView; }
  const GraphicsPipeline& getPipeline() const { return *pPipeline; }
  const vk::raii::Buffer& getCurrentUniformBuffer() const { return uniformBuffers[frameIndex]; }

  void setCommandBufferRecorder(CommandBufferRecorder recorder) {
    customRecorder = std::move(recorder);
  }

  void recreateSwapChain() {
    // Handle minimized windows
    int width = 0, height = 0;
    glfwGetFramebufferSize(pWindow, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(pWindow, &width, &height);
      glfwWaitEvents();
    }

    (*pLogicalDevice)->waitIdle();

    // Destroy old resources in correct order
    depthImageView = nullptr;
    depthImageMemory = nullptr;
    depthImage = nullptr;
    renderFinishedSemaphores.clear();
    pSwapChain.reset();

    // Recreate swapchain and dependent resources
    pSwapChain = std::make_unique<sauce::SwapChain>(
        *pPhysicalDevice, *pLogicalDevice, *pRenderSurface, pWindow
    );

    createDepthResources(*pPhysicalDevice, *pLogicalDevice);

    for (size_t i = 0; i < pSwapChain->getImages().size(); ++i) {
      renderFinishedSemaphores.emplace_back(**pLogicalDevice, vk::SemaphoreCreateInfo{});
    }
  }

  void createDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice) {
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
    };
    vk::DescriptorSetLayoutCreateInfo dsLayoutInfo {
      .bindingCount = 1,
      .pBindings = &uboLayoutBinding,
    };

    descriptorSetLayout = vk::raii::DescriptorSetLayout{ *logicalDevice, dsLayoutInfo };
  }

  void createDescriptorSets(const sauce::LogicalDevice& logicalDevice) {
    vk::DescriptorPoolSize poolSize {
      .type = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = sauce::Renderer::MAX_FRAMES_IN_FLIGHT,
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo {
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = sauce::Renderer::MAX_FRAMES_IN_FLIGHT,
      .poolSizeCount = 1,
      .pPoolSizes = &poolSize,
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
          vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
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
    const vk::raii::CommandBuffer& cmdBuf,
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

    cmdBuf.pipelineBarrier2(dependencyInfo);
  }

  void recordCommandBuffer(uint32_t imageIndex, sauce::ImGuiRenderer* imguiRenderer){
    commandBuffers[frameIndex].begin({});

    transitionImageLayout(
      commandBuffers[frameIndex],
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
        commandBuffers[frameIndex],
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

    commandBuffers[frameIndex].drawIndexed(indices.size(), 1, 0, 0, 0);

    // Render ImGui overlay
    if (imguiRenderer) {
      imguiRenderer->render(commandBuffers[frameIndex], imageIndex);
    }

    commandBuffers[frameIndex].endRendering();

    transitionImageLayout(
      commandBuffers[frameIndex],
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

  void drawFrame(const sauce::LogicalDevice& logicalDevice, const sauce::Scene& scene, sauce::ImGuiRenderer* imguiRenderer = nullptr){
    // Wait for the previous frame to finish rendering before submitting the next frame
    auto fenceResult = logicalDevice->waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to wait for fence!");
    }

    // Request the next available image from the swap chain
    uint32_t imageIndex;
    try {
      auto [result, idx] = (*pSwapChain)->acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
      if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
      }
      imageIndex = idx;
    } catch (const vk::OutOfDateKHRError&) {
      recreateSwapChain();
      return;
    }

    // Reset the fence for the next frame
    logicalDevice->resetFences(*inFlightFences[frameIndex]);

    // Reset and record the command buffer with rendering commands
    commandBuffers[frameIndex].reset();

    if (customRecorder) {
      customRecorder(commandBuffers[frameIndex], imageIndex);
    } else {
      recordCommandBuffer(imageIndex, imguiRenderer);
      updateUniformBuffer(frameIndex, scene);
    }

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

    // Present the rendered image to the screen, handling resize
    try {
      auto result = pQueue->presentKHR(presentInfoKHR);
      if (result == vk::Result::eSuboptimalKHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
      }
    } catch (const vk::OutOfDateKHRError&) {
      framebufferResized = false;
      recreateSwapChain();
    }

    // Advance to the next frame in the circular buffer
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
  }

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
  // Stored references for swapchain recreation
  const sauce::PhysicalDevice* pPhysicalDevice;
  const sauce::LogicalDevice* pLogicalDevice;
  const sauce::RenderSurface* pRenderSurface;
  GLFWwindow* pWindow;

  std::unique_ptr<vk::raii::Queue> pQueue;
  std::unique_ptr<sauce::SwapChain> pSwapChain;

  vk::raii::CommandPool commandPool = nullptr;

  std::vector<vk::raii::CommandBuffer> commandBuffers;

  std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  std::vector<vk::raii::Fence> inFlightFences;

  uint32_t frameIndex = 0;
  uint32_t queueIndex = ~0;
  bool framebufferResized = false;

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

  CommandBufferRecorder customRecorder;
};

}
