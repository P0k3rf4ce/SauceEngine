#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <chrono>
#include <functional>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/BufferUtils.hpp>
#include <app/Camera.hpp>
#include <app/components/LightComponent.hpp>
#include <app/GraphicsPipeline.hpp>
#include <app/IBLGenerator.hpp>
#include <app/ImGuiRenderer.hpp>
#include <app/ImageUtils.hpp>
#include <app/LogicalDevice.hpp>
#include <app/Scene.hpp>
#include <app/SwapChain.hpp>

#include <app/IBLGenerator.hpp>
#include <cstring>

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

// Callback type for custom command buffer recording
using CommandBufferRecorder = std::function<void(vk::raii::CommandBuffer&, uint32_t)>;

class Renderer {
public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
  static constexpr uint32_t MAX_LIGHTS = 64;

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
        createInfo.window,
        createInfo.vsync
    );


    createDescriptorSetLayout(createInfo.logicalDevice);
    
    sauce::GraphicsPipelineConfig mainPipelineConfig {
      .physicalDevice = createInfo.physicalDevice,
      .logicalDevice = createInfo.logicalDevice,
      .descriptorSetLayouts = { *descriptorSetLayout0, *descriptorSetLayout1, *modeling::Material::getDescriptorSetLayout() },
      .colorFormat = pSwapChain->getSurfaceFormat().format,
      .shaderPath = "shaders/shader_pbr.spv",
    };
    pPipeline = std::make_unique<sauce::GraphicsPipeline>(mainPipelineConfig);

    sauce::GraphicsPipelineConfig postProcessPipelineConfig {
      .physicalDevice = createInfo.physicalDevice,
      .logicalDevice = createInfo.logicalDevice,
      .descriptorSetLayouts = { *postProcessDescriptorSetLayout },
      .colorFormat = pSwapChain->getSurfaceFormat().format,
      .shaderPath = "shaders/postprocess.spv",
      .hasVertexInput = false,
      .depthTestEnable = false,
    };
    pPostProcessPipeline = std::make_unique<sauce::GraphicsPipeline>(postProcessPipelineConfig);

    vk::CommandPoolCreateInfo commandPoolCreateInfo {
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueIndex,
    };

    commandPool = vk::raii::CommandPool { *createInfo.logicalDevice, commandPoolCreateInfo };

    createDepthResources(createInfo.physicalDevice, createInfo.logicalDevice);
    createOffscreenResources(createInfo.physicalDevice, createInfo.logicalDevice);
    createDefaultTextures(createInfo.physicalDevice, createInfo.logicalDevice);
    createDefaultIBLTextures(createInfo.physicalDevice, createInfo.logicalDevice);
    createMaterialBuffer(createInfo.physicalDevice, createInfo.logicalDevice);
    createLightSSBO(createInfo.physicalDevice, createInfo.logicalDevice);


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
    createPostProcessDescriptorSets(createInfo.logicalDevice);

    for (size_t i = 0; i < pSwapChain->getImages().size(); ++i) {
      renderFinishedSemaphores.emplace_back(*createInfo.logicalDevice, vk::SemaphoreCreateInfo{});
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      presentCompleteSemaphores.emplace_back(*createInfo.logicalDevice, vk::SemaphoreCreateInfo{});
      inFlightFences.emplace_back(*createInfo.logicalDevice, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
    }
  }

  ~Renderer() {
    modeling::Material::cleanup();
  }

  const vk::raii::Queue& getQueue() const { return *pQueue; }
  const sauce::SwapChain& getSwapChain() const { return *pSwapChain; }
  const vk::raii::CommandPool& getCommandPool() const { return commandPool; }
  const vk::raii::DescriptorSetLayout& getDescriptorSetLayout0() const { return descriptorSetLayout0; }
  const vk::raii::DescriptorSetLayout& getDescriptorSetLayout1() const { return descriptorSetLayout1; }
  uint32_t getFrameIndex() const { return frameIndex; }
  const vk::raii::DescriptorSet& getCurrentDescriptorSet() const { return descriptorSets[frameIndex]; }
  const vk::raii::DescriptorSet& getEnvironmentDescriptorSet() const { 
    assert(!environmentDescriptorSets.empty() && "Environment descriptor sets not initialized!");
    return environmentDescriptorSets[0]; 
  }
  const vk::raii::DescriptorSet& getDefaultMaterialDescriptorSet() const { 
    assert(!defaultMaterialDescriptorSets.empty() && "Default material descriptor sets not initialized!");
    return defaultMaterialDescriptorSets[0]; 
  }
  void* getCurrentUniformBufferMapped() const { return uniformBuffersMapped[frameIndex]; }

  void setFramebufferResized() { framebufferResized = true; }

  const vk::raii::Image& getDepthImage() const { return depthImage; }
  const vk::raii::ImageView& getDepthImageView() const { return depthImageView; }
  const GraphicsPipeline& getPipeline() const { return *pPipeline; }
  const vk::raii::Buffer& getCurrentUniformBuffer() const { return uniformBuffers[frameIndex]; }
  const vk::raii::DescriptorPool& getDescriptorPool() const { return descriptorPool; }
  const vk::raii::ImageView& getDefaultImageView() const { return defaultImageView; }
  const vk::raii::Sampler& getDefaultSampler() const { return defaultSampler; }

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
    // Set 0: Per-Frame Layout
    std::array<vk::DescriptorSetLayoutBinding, 2> perFrameBindings;
    // Camera UBO
    perFrameBindings[0] = { .binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer,  .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment };
    // Light SSBO
    perFrameBindings[1] = { .binding = 1, .descriptorType = vk::DescriptorType::eStorageBuffer,  .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment };

    vk::DescriptorSetLayoutCreateInfo perFrameDsLayoutInfo {
      .bindingCount = static_cast<uint32_t>(perFrameBindings.size()),
      .pBindings = perFrameBindings.data(),
    };
    descriptorSetLayout0 = vk::raii::DescriptorSetLayout{ *logicalDevice, perFrameDsLayoutInfo };

    // Set 1: Environment Layout (IBL Maps)
    std::array<vk::DescriptorSetLayoutBinding, 3> environmentBindings;
    // IBL Maps: Irradiance, Prefilter, BRDF LUT
    environmentBindings[0] = { .binding = 0, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment };
    environmentBindings[1] = { .binding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment };
    environmentBindings[2] = { .binding = 2, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment };

    vk::DescriptorSetLayoutCreateInfo environmentDsLayoutInfo {
      .bindingCount = static_cast<uint32_t>(environmentBindings.size()),
      .pBindings = environmentBindings.data(),
    };
    descriptorSetLayout1 = vk::raii::DescriptorSetLayout{ *logicalDevice, environmentDsLayoutInfo };

    modeling::Material::initDescriptorSetLayout(logicalDevice);

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
    std::array<vk::DescriptorPoolSize, 3> poolSizes {{
      { vk::DescriptorType::eUniformBuffer, 1024u },
      { vk::DescriptorType::eStorageBuffer, 1024u },
      { vk::DescriptorType::eCombinedImageSampler, 1024u },
    }};

    vk::DescriptorPoolCreateInfo poolCreateInfo {
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = 1024u,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data(),
    };

    descriptorPool = vk::raii::DescriptorPool{ *logicalDevice, poolCreateInfo };

    // Per-frame sets
    std::vector<vk::DescriptorSetLayout> layouts{ MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout0 };
    vk::DescriptorSetAllocateInfo dsAllocInfo {
      .descriptorPool = descriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()
    };
    descriptorSets = logicalDevice->allocateDescriptorSets(dsAllocInfo);

    // Environment set
    vk::DescriptorSetAllocateInfo envAllocInfo {
      .descriptorPool = descriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &*descriptorSetLayout1
    };
    environmentDescriptorSets = logicalDevice->allocateDescriptorSets(envAllocInfo);

    // Default material set
    vk::DescriptorSetLayout materialLayout = *modeling::Material::getDescriptorSetLayout();
    vk::DescriptorSetAllocateInfo matAllocInfo {
      .descriptorPool = descriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &materialLayout
    };
    defaultMaterialDescriptorSets = logicalDevice->allocateDescriptorSets(matAllocInfo);

    vk::DescriptorBufferInfo lightSSBOInfo { .buffer = *lightSSBO, .offset = 0, .range = lightSSBOSize };
    
    // Fallback for IBL cubemaps (irradiance, prefilter) — use default black cubemap
    vk::DescriptorImageInfo iblCubemapInfo { .sampler = *iblSampler, .imageView = *defaultCubemapView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    // Fallback for BRDF LUT — use default 2D image
    vk::DescriptorImageInfo iblLutInfo { .sampler = *iblSampler, .imageView = *defaultImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vk::DescriptorBufferInfo uboInfo { .buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject) };

      std::array<vk::WriteDescriptorSet, 2> writes;
      writes[0] = { .dstSet = descriptorSets[i], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer,  .pBufferInfo = &uboInfo };
      writes[1] = { .dstSet = descriptorSets[i], .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer,  .pBufferInfo = &lightSSBOInfo };

      logicalDevice->updateDescriptorSets(writes, {});
    }

    std::array<vk::WriteDescriptorSet, 3> envWrites;
    envWrites[0] = { .dstSet = environmentDescriptorSets[0], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &iblCubemapInfo };
    envWrites[1] = { .dstSet = environmentDescriptorSets[0], .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &iblCubemapInfo };
    envWrites[2] = { .dstSet = environmentDescriptorSets[0], .dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &iblLutInfo };
    logicalDevice->updateDescriptorSets(envWrites, {});

    vk::DescriptorBufferInfo matUboInfo { .buffer = *materialBuffer, .offset = 0, .range = sizeof(MaterialData) };
    vk::DescriptorImageInfo defaultImageInfo { .sampler = *defaultSampler, .imageView = *defaultImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

    std::array<vk::WriteDescriptorSet, 6> matWrites;
    for (uint32_t i = 0; i < 5; ++i) {
      matWrites[i] = { .dstSet = defaultMaterialDescriptorSets[0], .dstBinding = i, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &defaultImageInfo };
    }
    matWrites[5] = { .dstSet = defaultMaterialDescriptorSets[0], .dstBinding = 5, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &matUboInfo };
    logicalDevice->updateDescriptorSets(matWrites, {});
  }

  void createPostProcessDescriptorSets(const sauce::LogicalDevice& logicalDevice) {
    std::vector<vk::DescriptorSetLayout> layouts{ 1, *postProcessDescriptorSetLayout };
    vk::DescriptorSetAllocateInfo dsAllocInfo {
      .descriptorPool = descriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()
    };

    postProcessDescriptorSets = logicalDevice->allocateDescriptorSets(dsAllocInfo);

    vk::DescriptorImageInfo imageInfo {
      .sampler = *offscreenSampler,
      .imageView = *offscreenImageView,
      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };

    vk::WriteDescriptorSet descriptorWrite {
      .dstSet = postProcessDescriptorSets[0],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = &imageInfo,
    };

    logicalDevice->updateDescriptorSets(descriptorWrite, {});
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

    // Transition offscreen image for rendering
    transitionImageLayout(
      commandBuffers[frameIndex],
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
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pPipeline->getLayout(), 1, *environmentDescriptorSets[0], nullptr);
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pPipeline->getLayout(), 2, *defaultMaterialDescriptorSets[0], nullptr);

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

    commandBuffers[frameIndex].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    commandBuffers[frameIndex].endRendering();

    transitionImageLayout(
      commandBuffers[frameIndex],
      *offscreenImage,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::AccessFlagBits2::eShaderRead,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eFragmentShader,
      vk::ImageAspectFlagBits::eColor
    );

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
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pPostProcessPipeline->getLayout(), 0, *postProcessDescriptorSets[0], nullptr);

    commandBuffers[frameIndex].setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(pSwapChain->getExtent().width), 
        static_cast<float>(pSwapChain->getExtent().height), 0.0f, 1.0f));
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), pSwapChain->getExtent()));

    commandBuffers[frameIndex].draw(3, 1, 0, 0);

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

  // Creates a 1x1 black cubemap and a linear-clamp sampler used as fallbacks for IBL textures.
  void createDefaultIBLTextures(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice) {
    // Create 1x1 cubemap (6 faces) with black pixels as default irradiance/prefilter
    uint8_t blackPixel[4] = {0, 0, 0, 255};
    vk::DeviceSize faceSize = sizeof(blackPixel);
    vk::DeviceSize totalSize = faceSize * 6;

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingMemory = nullptr;
    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice, totalSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingMemory
    );
    void* mapped = stagingMemory.mapMemory(0, totalSize);
    for (uint32_t face = 0; face < 6; ++face) {
      memcpy(static_cast<uint8_t*>(mapped) + face * faceSize, blackPixel, faceSize);
    }
    stagingMemory.unmapMemory();

    ImageUtils::createImage(
        physicalDevice, logicalDevice, 1, 1,
        vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        defaultCubemap, defaultCubemapMemory,
        1, 6, vk::ImageCreateFlagBits::eCubeCompatible
    );

    ImageUtils::transitionImageLayout(
        logicalDevice, commandPool, *pQueue, defaultCubemap,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
        {}, vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eTransfer,
        1, 6
    );

    // Copy each face from staging buffer using a single-time command buffer
    {
      vk::CommandBufferAllocateInfo allocInfo {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
      };
      vk::raii::CommandBuffer cmd = std::move(logicalDevice->allocateCommandBuffers(allocInfo).front());
      cmd.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

      std::array<vk::BufferImageCopy, 6> copyRegions;
      for (uint32_t face = 0; face < 6; ++face) {
        copyRegions[face] = {
          .bufferOffset = face * faceSize,
          .bufferRowLength = 0,
          .bufferImageHeight = 0,
          .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = face,
            .layerCount = 1,
          },
          .imageOffset = {0, 0, 0},
          .imageExtent = {1, 1, 1},
        };
      }
      cmd.copyBufferToImage(*stagingBuffer, *defaultCubemap, vk::ImageLayout::eTransferDstOptimal, copyRegions);

      cmd.end();
      pQueue->submit(vk::SubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmd,
      }, nullptr);
      pQueue->waitIdle();
    }

    ImageUtils::transitionImageLayout(
        logicalDevice, commandPool, *pQueue, defaultCubemap,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite, vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eFragmentShader,
        1, 6
    );

    defaultCubemapView = ImageUtils::createImageView(
        logicalDevice, defaultCubemap, vk::Format::eR8G8B8A8Unorm,
        vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube, 1, 6
    );

    // IBL sampler: linear filtering, clamp-to-edge
    vk::SamplerCreateInfo iblSamplerInfo {
      .magFilter = vk::Filter::eLinear,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .addressModeU = vk::SamplerAddressMode::eClampToEdge,
      .addressModeV = vk::SamplerAddressMode::eClampToEdge,
      .addressModeW = vk::SamplerAddressMode::eClampToEdge,
      .minLod = 0.0f,
      .maxLod = 4.0f, // prefilter map has 5 mip levels
    };
    iblSampler = vk::raii::Sampler{ *logicalDevice, iblSamplerInfo };
  }

  // Updates the IBL descriptor bindings with real IBL maps from IBLGenerator.
  void setIBLMaps(const sauce::LogicalDevice& logicalDevice, const IBLMaps& maps) {
    vk::DescriptorImageInfo irradianceInfo { .sampler = *maps.sampler, .imageView = *maps.irradianceMapView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    vk::DescriptorImageInfo prefilterInfo  { .sampler = *maps.sampler, .imageView = *maps.prefilterMapView,  .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
    vk::DescriptorImageInfo brdfLUTInfo    { .sampler = *maps.sampler, .imageView = *maps.brdfLUTView,       .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

    std::array<vk::WriteDescriptorSet, 3> writes;
    writes[0] = { .dstSet = environmentDescriptorSets[0], .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &irradianceInfo };
    writes[1] = { .dstSet = environmentDescriptorSets[0], .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &prefilterInfo };
    writes[2] = { .dstSet = environmentDescriptorSets[0], .dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &brdfLUTInfo };

    logicalDevice->updateDescriptorSets(writes, {});
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

  // Pre-allocates a persistently-mapped storage buffer for up to MAX_LIGHTS.
  void createLightSSBO(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice) {
    lightSSBOSize = static_cast<vk::DeviceSize>(MAX_LIGHTS) * sizeof(GPULight);
    sauce::BufferUtils::createBuffer(
        physicalDevice, logicalDevice, lightSSBOSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        lightSSBO, lightSSBOMemory
    );
    lightSSBOMapped = lightSSBOMemory.mapMemory(0, lightSSBOSize);
  }

  // Writes lights into the persistently-mapped SSBO. Returns count written (clamped to MAX_LIGHTS).
  uint32_t updateLightSSBO(const GPULight* data, uint32_t count) {
    count = std::min(count, MAX_LIGHTS);
    if (count > 0) {
      std::memcpy(lightSSBOMapped, data, count * sizeof(GPULight));
    }
    return count;
  }

  const vk::raii::Buffer& getMaterialBuffer() const { return materialBuffer; }

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
  std::unique_ptr<sauce::GraphicsPipeline> pPostProcessPipeline;

  vk::raii::DescriptorSetLayout descriptorSetLayout0 = nullptr;
  vk::raii::DescriptorSetLayout descriptorSetLayout1 = nullptr;
  vk::raii::DescriptorSetLayout postProcessDescriptorSetLayout = nullptr;
  vk::raii::DescriptorPool descriptorPool = nullptr;
  std::vector<vk::raii::DescriptorSet> descriptorSets;
  std::vector<vk::raii::DescriptorSet> environmentDescriptorSets;
  std::vector<vk::raii::DescriptorSet> defaultMaterialDescriptorSets;
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

  CommandBufferRecorder customRecorder;

  // PBR resources
  vk::raii::Buffer materialBuffer = nullptr;
  vk::raii::DeviceMemory materialBufferMemory = nullptr;

  vk::DeviceSize lightSSBOSize{0};
  vk::raii::Buffer lightSSBO = nullptr;
  vk::raii::DeviceMemory lightSSBOMemory = nullptr;
  void* lightSSBOMapped = nullptr;

  vk::raii::Image defaultImage = nullptr;
  vk::raii::DeviceMemory defaultImageMemory = nullptr;
  vk::raii::ImageView defaultImageView = nullptr;
  vk::raii::Sampler defaultSampler = nullptr;

  vk::raii::Image offscreenImage = nullptr;
  vk::raii::DeviceMemory offscreenImageMemory = nullptr;
  vk::raii::ImageView offscreenImageView = nullptr;
  vk::raii::Sampler offscreenSampler = nullptr;

  // Default IBL resources (black cubemap + linear-clamp sampler)
  vk::raii::Image defaultCubemap = nullptr;
  vk::raii::DeviceMemory defaultCubemapMemory = nullptr;
  vk::raii::ImageView defaultCubemapView = nullptr;
  vk::raii::Sampler iblSampler = nullptr;

  std::unique_ptr<IBLMaps> pIBLMaps;
};

}
