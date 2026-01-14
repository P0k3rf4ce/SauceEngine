#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <app/Instance.hpp>
#include <app/PhysicalDevice.hpp>
#include <app/LogicalDevice.hpp>
#include <app/RenderSurface.hpp>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

constexpr uint32_t WIDTH = 1280;
constexpr uint32_t HEIGHT = 720;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
  }

  static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription() {
    return {
      vk::VertexInputAttributeDescription { 0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos) },
      vk::VertexInputAttributeDescription { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) },
    };
  }
};

const std::vector<Vertex> vertices {
  { {-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },
  { {0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },
  { {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} },
  { {-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f} }
};

const std::vector<uint16_t> indices {
  0, 1, 2, 2, 3, 0
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class SauceEngineApp {
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
  }

  ~SauceEngineApp() {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

private:
  GLFWwindow *window;

  std::unique_ptr<sauce::Instance> pInstance;

  std::unique_ptr<sauce::RenderSurface> renderSurface;

  sauce::PhysicalDevice physicalDevice = nullptr;
  sauce::LogicalDevice logicalDevice = nullptr;
  uint32_t queueIndex = ~0;
  vk::raii::Queue queue = nullptr;

  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  vk::SurfaceFormatKHR swapChainSurfaceFormat;
  vk::Extent2D swapChainExtent;
  std::vector<vk::raii::ImageView> swapChainImageViews;

  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;

  vk::raii::PipelineLayout pipelineLayout = nullptr;

  vk::raii::Pipeline graphicsPipeline = nullptr;

  vk::raii::CommandPool commandPool = nullptr;

  vk::raii::Buffer vertexBuffer = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory = nullptr;
  vk::raii::Buffer indexBuffer = nullptr;
  vk::raii::DeviceMemory indexBufferMemory = nullptr;

  std::vector<vk::raii::Buffer> uniformBuffers;
  std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
  std::vector<void *> uniformBuffersMapped;

  vk::raii::DescriptorPool descriptorPool = nullptr;
  std::vector<vk::raii::DescriptorSet> descriptorSets;

  std::vector<vk::raii::CommandBuffer> commandBuffers;

  std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  std::vector<vk::raii::Fence> inFlightFences;

  uint32_t frameIndex = 0;

  bool framebufferResized = false;

  void initVulkan() {
    createInstance(); // DONE
    createSurface(); // DONE
    pickPhysicalDevice(); // DONE 
    createLogicalDevice(); // DONE 
    createSwapChain(); // TODO: Renderer
    createImageViews(); // TODO: Renderer
    createDescriptorSetLayout(); // Keep here
    createGraphicsPipeline(); // TODO: GraphicsPipeline 
    createCommandPool(); // TODO: Renderer 
    createVertexBuffer(); // Keep here 
    createIndexBuffer(); // Keep here 
    createUniformBuffers(); // Keep here 
    createDescriptorPool(); // Keep here 
    createDescriptorSets(); // Keep here 
    createCommandBuffers(); // TODO: Renderer 
    createSyncObjects(); // TODO: Renderer
  }

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Playground", nullptr, nullptr);
  }

  void createInstance() {
    uint32_t glfwExtensionsCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    pInstance = std::make_unique<sauce::Instance>(glfwExtensions, glfwExtensionsCount);
  }

  void createSurface() {
    renderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);
  }

  void pickPhysicalDevice() {
    physicalDevice = { *pInstance };
  }

  void createLogicalDevice() {
    logicalDevice = { physicalDevice, *renderSurface };
    queueIndex = logicalDevice.getQueueIndex();
    queue = vk::raii::Queue { *logicalDevice, queueIndex, 0 };
  }

  static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& format: availableFormats) {
      if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        return format;
    }

    return availableFormats[0];
  }

  static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& mode: availablePresentModes) {
      if (mode == vk::PresentModeKHR::eMailbox) {
        return mode;
      }
    }

    assert(std::ranges::any_of(availablePresentModes, [](const vk::PresentModeKHR& mode) { return mode == vk::PresentModeKHR::eFifo; }));

    return vk::PresentModeKHR::eFifo; // Should always be supported
  }

  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
      return capabilities.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
      std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
      std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
  }

  static uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities) {
    auto minImageCount = std::max(3u, capabilities.minImageCount);
    if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < minImageCount)) {
      minImageCount = capabilities.maxImageCount;
    }
    return minImageCount;
  }

  void createSwapChain() {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice->getSurfaceCapabilitiesKHR(**renderSurface);
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicalDevice->getSurfaceFormatsKHR(**renderSurface));
    vk::SwapchainCreateInfoKHR swapChainCreateInfo {
      .surface = **renderSurface,
      .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
      .imageFormat = swapChainSurfaceFormat.format,
      .imageColorSpace = swapChainSurfaceFormat.colorSpace,
      .imageExtent = swapChainExtent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = chooseSwapPresentMode(physicalDevice->getSurfacePresentModesKHR(**renderSurface)),
      .clipped = vk::True
    };

    swapChain = vk::raii::SwapchainKHR { *logicalDevice, swapChainCreateInfo };
    swapChainImages = swapChain.getImages();
  }

  void createImageViews() {
    assert(swapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo {
      .viewType = vk::ImageViewType::e2D,
      .format = swapChainSurfaceFormat.format,
      .subresourceRange = {
        vk::ImageAspectFlagBits::eColor,
        0, 1, 0, 1,
      },
    };

    for (auto& image: swapChainImages) {
      imageViewCreateInfo.image = image;
      swapChainImageViews.emplace_back( *logicalDevice, imageViewCreateInfo );
    }
  }

  void createDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
      .binding = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
    };
    vk::DescriptorSetLayoutCreateInfo dsLayoutInfo {
      .bindingCount = 1,
      .pBindings = &uboLayoutBinding,
    };

    descriptorSetLayout = vk::raii::DescriptorSetLayout{ *logicalDevice, dsLayoutInfo };
  }

  static std::vector<char> readBinaryFile(const std::string filename) {
    std::ifstream file { filename, std::ios::binary | std::ios::ate };

    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file: " + filename);
    }

    std::vector<char> buf ( file.tellg() );

    file.seekg(0, std::ios::beg);
    file.read(buf.data(), static_cast<std::streamsize>(buf.size()));

    file.close();

    return buf;
  }

  [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo {
      .codeSize = code.size() * sizeof(char),
      .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    return { *logicalDevice, shaderModuleCreateInfo };
  }

  void createGraphicsPipeline() {
    vk::raii::ShaderModule shaderModule = createShaderModule(readBinaryFile("shaders/slang.spv"));
    vk::PipelineShaderStageCreateInfo vertShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = shaderModule,
      .pName = "vertMain",
    };
    vk::PipelineShaderStageCreateInfo fragShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = shaderModule,
      .pName = "fragMain",
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
      vertShaderCreateInfo,
      fragShaderCreateInfo,
    };
    
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescription();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &bindingDescription,
      .vertexAttributeDescriptionCount = attributeDescriptions.size(),
      .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
      .topology = vk::PrimitiveTopology::eTriangleList,
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo {
      .viewportCount = 1,
      .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizerInfo {
      .depthClampEnable = vk::False,
      .rasterizerDiscardEnable = vk::False,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eCounterClockwise,
      .depthBiasEnable = vk::False,
      .depthBiasSlopeFactor = 1.0f,
      .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisamplingInfo {
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
      .blendEnable = vk::False,
      .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendInfo {
      .logicOpEnable = vk::False,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
    };

    std::vector<vk::DynamicState> dynamicStates {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo {
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
      .setLayoutCount = 1,
      .pSetLayouts = &*descriptorSetLayout,
      .pushConstantRangeCount = 0,
    };

    pipelineLayout = vk::raii::PipelineLayout { *logicalDevice, pipelineLayoutInfo };

    vk::PipelineRenderingCreateInfo renderingCreateInfo {
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo {
      .pNext = &renderingCreateInfo,
      .stageCount = 2,
      .pStages = shaderStages,
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssemblyInfo,
      .pViewportState = &viewportStateInfo,
      .pRasterizationState = &rasterizerInfo,
      .pMultisampleState = &multisamplingInfo,
      .pColorBlendState = &colorBlendInfo,
      .pDynamicState = &dynamicStateInfo,
      .layout = pipelineLayout,
      .renderPass = nullptr,
    };

    graphicsPipeline = vk::raii::Pipeline { *logicalDevice, nullptr, pipelineInfo };
  }

  void createCommandPool() {
    vk::CommandPoolCreateInfo commandPoolCreateInfo {
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueIndex,
    };

    commandPool = vk::raii::CommandPool { *logicalDevice, commandPoolCreateInfo };
  }

  uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties deviceMemoryProps = physicalDevice->getMemoryProperties();

    for (uint32_t i = 0; i < deviceMemoryProps.memoryTypeCount; ++i) {
      if ((typeFilter & (1 << i)) && (deviceMemoryProps.memoryTypes[i].propertyFlags & properties) == properties)
        return i;
    }

    throw std::runtime_error("Failed to find suitable memory type!");
  }

  void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory) {
    vk::BufferCreateInfo bufferCreateInfo {
      .size = size,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
    };

    buffer = vk::raii::Buffer { *logicalDevice, bufferCreateInfo };

    vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo memoryAllocateInfo {
      .allocationSize = memoryRequirements.size,
      .memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, props), 
    };

    bufferMemory = vk::raii::DeviceMemory{ *logicalDevice, memoryAllocateInfo };
    buffer.bindMemory(*bufferMemory, 0);
  }

  void copyBuffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize size) {
    vk::CommandBufferAllocateInfo copyCommandBufferAllocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
    };
    vk::raii::CommandBuffer copyCommandBuffer = std::move(logicalDevice->allocateCommandBuffers(copyCommandBufferAllocInfo).front());

    copyCommandBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    copyCommandBuffer.copyBuffer(src, dst, vk::BufferCopy(0, 0, size));
    copyCommandBuffer.end();

    queue.submit(vk::SubmitInfo{ 
      .commandBufferCount = 1,
      .pCommandBuffers = &*copyCommandBuffer,
    }, nullptr);
    queue.waitIdle();
  }

  void createVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
        stagingBuffer,
        stagingBufferMemory
    );

    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    createBuffer(
        bufferSize, 
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer, 
        vertexBufferMemory
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
  }

  void createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
        stagingBuffer,
        stagingBufferMemory
    );

    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, indices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    createBuffer(
        bufferSize, 
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        indexBuffer, 
        indexBufferMemory
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
  }

  void createUniformBuffers() {
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersMapped.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vk::DeviceSize size = sizeof(UniformBufferObject);
      vk::raii::Buffer buf = nullptr;
      vk::raii::DeviceMemory mem = nullptr;
      createBuffer(
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

  void createDescriptorPool() {
    vk::DescriptorPoolSize poolSize {
      .descriptorCount = MAX_FRAMES_IN_FLIGHT,
      .type = vk::DescriptorType::eUniformBuffer,
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo {
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
      .poolSizeCount = 1,
      .pPoolSizes = &poolSize,
    };

    descriptorPool = vk::raii::DescriptorPool{ *logicalDevice, poolCreateInfo };
  }

  void createDescriptorSets() {
    descriptorSets.clear();

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

  void createCommandBuffers() {
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    commandBuffers = vk::raii::CommandBuffers(*logicalDevice, allocInfo);
  }

  void transitionImageLayout(
    uint32_t imageIndex,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask
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
      .image = swapChainImages[imageIndex],
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
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

  void recordCommandBuffer(uint32_t imageIndex) {
    commandBuffers[frameIndex].begin({});

    transitionImageLayout(
      imageIndex,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eColorAttachmentOptimal,
      {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    vk::ClearValue clearColor = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };
    vk::RenderingAttachmentInfo attachmentInfo = {
      .imageView = swapChainImageViews[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearColor,
    };

    vk::RenderingInfo renderingInfo {
      .renderArea = { 
        .offset = { 0, 0 }, 
        .extent = swapChainExtent,
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &attachmentInfo,
    };

    commandBuffers[frameIndex].beginRendering(renderingInfo);

    commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
    commandBuffers[frameIndex].bindVertexBuffers(0, *vertexBuffer, {0});
    commandBuffers[frameIndex].bindIndexBuffer( *indexBuffer, 0, vk::IndexType::eUint16 );
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[frameIndex], nullptr);

    commandBuffers[frameIndex].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));

    commandBuffers[frameIndex].drawIndexed(indices.size(), 1, 0, 0, 0);

    commandBuffers[frameIndex].endRendering();

    transitionImageLayout(
      imageIndex,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffers[frameIndex].end();
  }

  void createSyncObjects() {
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
      renderFinishedSemaphores.emplace_back(*logicalDevice, vk::SemaphoreCreateInfo{});
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      presentCompleteSemaphores.emplace_back(*logicalDevice, vk::SemaphoreCreateInfo{});
      inFlightFences.emplace_back(*logicalDevice, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
    }
  }

  void updateUniformBuffer(uint32_t curImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo {
      .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
      .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.1f)),
      .proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f),
    };
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[curImage], &ubo, sizeof(ubo));
  }

  void drawFrame() {
    auto fenceResult = logicalDevice->waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to wait for fence!");
    }

    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
      recreateSwapChain();
      return;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("Failed to acquire swap chain image");
    }

    logicalDevice->resetFences(*inFlightFences[frameIndex]);


    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);

    updateUniformBuffer(frameIndex);

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

    queue.submit(submitInfo, *inFlightFences[frameIndex]);

    try{
      const vk::PresentInfoKHR presentInfoKHR {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain,
        .pImageIndices = &imageIndex,
      };

      result = queue.presentKHR(presentInfoKHR);
      if (result == vk::Result::eSuboptimalKHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
      } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present swap chain image!");
      }
    } catch (const vk::SystemError& e) {
      if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) {
        recreateSwapChain();
        return;
      } else throw;
    }

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void cleanupSwapChain() {
    swapChainImageViews.clear();
    swapChain = nullptr;
  }

  void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
    }

    logicalDevice->waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      drawFrame();
    }

    logicalDevice->waitIdle();
  }
};

