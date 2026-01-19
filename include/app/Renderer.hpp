#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/BufferUtils.hpp>
#include <app/GraphicsPipeline.hpp>
#include <app/LogicalDevice.hpp>
#include <app/SwapChain.hpp>

namespace sauce {

const std::vector<Vertex> vertices {
  { {-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },
  { {0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },
  { {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} },
  { {-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f} }
};

const std::vector<uint16_t> indices {
  0, 1, 2, 2, 3, 0
};

struct Renderer {

  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

/**
 * @brief Constructs the Renderer and initializes all Vulkan rendering resources.
 *
 * This constructor sets up the complete rendering pipeline by:
 * 1. Creating a graphics queue for command submission.
 * 2. Initializing the swap chain for presenting images to the screen.
 * 3. Setting up descriptor set layouts for uniform buffer access.
 * 4. Building the graphics pipeline with shaders and render state.
 * 5. Creating command pools and buffers for recording GPU commands.
 * 6. Allocating uniform, vertex, and index buffers for scene data.
 * 7. Configuring descriptor sets to bind uniform buffers to shaders.
 * 8. Creating synchronization primitives (semaphores and fences) for frame coordination.
 *
 * @param physicalDevice The GPU being used for rendering operations.
 * @param logicalDevice The logical device interface for resource creation.
 * @param renderSurface The window surface where images will be presented.
 * @param window GLFW window handle for querying window properties.
 *
 * @throws std::runtime_error If any Vulkan resource fails to initialize.
 */
Renderer(
    const sauce::PhysicalDevice& physicalDevice,
    const sauce::LogicalDevice& logicalDevice,
    const sauce::RenderSurface& renderSurface,
    GLFWwindow* window
) {
    queueIndex = logicalDevice.getQueueIndex();
    pQueue = std::make_unique<vk::raii::Queue>(*logicalDevice, queueIndex, 0);

    pSwapChain = std::make_unique<sauce::SwapChain>(physicalDevice, logicalDevice, renderSurface, window);


    createDescriptorSetLayout(logicalDevice);
    pPipeline = std::make_unique<sauce::GraphicsPipeline>(logicalDevice, descriptorSetLayout, *pSwapChain);

    vk::CommandPoolCreateInfo commandPoolCreateInfo {
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueIndex,
    };

    commandPool = vk::raii::CommandPool { *logicalDevice, commandPoolCreateInfo };
  
    vk::CommandBufferAllocateInfo allocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    commandBuffers = vk::raii::CommandBuffers(*logicalDevice, allocInfo);

    createUniformBuffers(physicalDevice, logicalDevice);
    createVertexBuffer(physicalDevice, logicalDevice);
    createIndexBuffer(physicalDevice, logicalDevice);

    createDescriptorSets(logicalDevice);

    for (size_t i = 0; i < pSwapChain->getImages().size(); ++i) {
      renderFinishedSemaphores.emplace_back(*logicalDevice, vk::SemaphoreCreateInfo{});
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      presentCompleteSemaphores.emplace_back(*logicalDevice, vk::SemaphoreCreateInfo{});
      inFlightFences.emplace_back(*logicalDevice, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
    }
  }

  /**
   * @brief Defines the blueprint for resources accessible by the shaders.
   *
   * A Descriptor Set Layout describes the "shape" of the data that will be bound to the pipeline.
   * It acts like a function signature for the shader.
   *
   * In this specific implementation, we define a single binding:
   * - **Binding 0:** A Uniform Buffer Object (UBO).
   * - **Stage:** Accessible in the Vertex Shader.
   * - **Count:** 1 descriptor (single UBO struct).
   *
   * @param logicalDevice The device used to create the layout handle.
   */
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
  }

  /**
   * @brief Allocates and configures descriptor sets to bind specific buffers to the layout.
   *
   * This function performs three main steps:
   * 1. **Create Pool:** Creates a `vk::DescriptorPool` large enough to hold the descriptors for all frames in flight.
   * 2. **Allocate Sets:** Allocates one `vk::DescriptorSet` per frame using the layout defined in `createDescriptorSetLayout`.
   * 3. **Update Sets:** Populates the allocated sets by pointing them to the actual `uniformBuffers` created in memory.
   *
   * This links the logical "Binding 0" in the shader to the specific memory address of the UBO on the GPU.
   *
   * @param logicalDevice The device used for pool creation and set allocation/updating.
   */
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
/**
 * @brief Creates uniform buffers for storing per-frame transformation data.
 *
 * This function allocates one uniform buffer per frame in flight, ensuring that
 * each frame has its own dedicated memory for the Model-View-Projection matrices.
 * The buffers are created with host-visible and host-coherent memory properties,
 * allowing direct CPU writes without explicit flush operations.
 *
 * Each buffer is persistently mapped to CPU-accessible memory for efficient updates
 * during rendering. This eliminates the need for map/unmap operations every frame.
 *
 * @param physicalDevice The GPU used to query memory properties.
 * @param logicalDevice The device used to create buffers and allocate memory.
 *
 * @throws std::runtime_error If buffer creation or memory allocation fails.
 */
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
/**
 * @brief Creates and initializes the vertex buffer containing mesh geometry data.
 *
 * This function uses a staging buffer approach for optimal GPU performance:
 * 1. Creates a host-visible staging buffer and copies vertex data from CPU memory.
 * 2. Creates a device-local vertex buffer optimized for GPU access.
 * 3. Issues a GPU copy command to transfer data from staging to device-local memory.
 * 4. Staging buffer is automatically destroyed when it goes out of scope.
 *
 * Device-local memory provides faster access speeds for the GPU compared to
 * host-visible memory, improving rendering performance.
 *
 * @param physicalDevice The GPU used to query memory properties.
 * @param logicalDevice The device used to create buffers and issue transfer commands.
 *
 * @throws std::runtime_error If buffer creation or memory transfer fails.
 */
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

/**
 * @brief Creates and initializes the index buffer for indexed drawing.
 *
 * This function mirrors the vertex buffer creation process with a staging approach:
 * 1. Allocates a host-visible staging buffer and uploads index data from CPU memory.
 * 2. Creates a device-local index buffer for optimal GPU read performance.
 * 3. Transfers index data from the staging buffer to device-local memory via GPU copy.
 * 4. Staging resources are cleaned up automatically after the transfer completes.
 *
 * Index buffers enable efficient mesh rendering by referencing vertices through indices
 * rather than duplicating vertex data, reducing memory usage and bandwidth.
 *
 * @param physicalDevice The GPU used to determine appropriate memory types.
 * @param logicalDevice The device used for buffer creation and copy operations.
 *
 * @throws std::runtime_error If buffer allocation or data transfer fails.
 */
void createIndexBuffer(
    const sauce::PhysicalDevice& physicalDevice,
    const sauce::LogicalDevice& logicalDevice
){
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

  /**
   * @brief Transitions a swapchain image from one layout to another.
   *
   * This function inserts a pipeline barrier specifying:
   * 1. How the image was accessed previously.
   * 2. How the image will be accessed next.
   * 3. Which parts of the image are affected.
   *
   * @param imageIndex Index of the swapchain image being transitioned.
   * @param oldLayout The image's current layout before the transition.
   * @param newLayout The layout the image must be in for the next operation.
   * @param srcAccessMask Types of memory access performed before the barrier.
   * @param dstAccessMask Types of memory access required after the barrier.
   * @param srcStageMask Pipeline stage where the previous access occurred.
   * @param dstStageMask Earliest pipeline stage where the next access may occur.
   *
   * @throws std::runtime_error If the command buffer is not in the recording state.
   */
   void transitionImageLayout(
    uint32_t imageIndex, 
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask, 
    vk::AccessFlags2 dstAccessMask, 
    vk::PipelineStageFlags2 srcStageMask, 
    vk::PipelineStageFlags2 dstStageMask
  ) {

    // Describe how the image was being used before, what we will use it for next, and what 
    // part of the image will be impacted, using the given parameters. 
    vk::ImageMemoryBarrier2 barrier {
      .srcStageMask = srcStageMask,
      .srcAccessMask = srcAccessMask,
      .dstStageMask = dstStageMask,
      .dstAccessMask = dstAccessMask,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = pSwapChain->getImages()[imageIndex],
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    };

    // Create a barrier for this image so we can finish up the previous work
    // so we can do other things with it safely
    vk::DependencyInfo dependencyInfo {
      .dependencyFlags = {},
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &barrier,
    };

    // Set the pipeline barrier for the image
    commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
  }

  /**
   * @brief Records all GPU commands required to render a single frame.
   *
   * This function builds the full command buffer for one frame by completing the following:
   * 1. Transition the swapchain image into a layout for rendering.
   * 2. Start a dynamic rendering pass after clearing the image.
   * 3. Bind the graphics pipeline, vertex/index buffers, and descriptor sets.
   * 4. Set the viewport and scissor state.
   * 5. Perform a draw call for the current mesh.
   * 6. Finish the rendering pass and mark the image for presentation.
   *
   * The resulting command buffer can be submitted to the graphics queue.
   *
   * @param imageIndex Index of the swapchain image that this frame will render into.
   *
   * @throws std::runtime_error If command buffer recording fails or if the image
   *         cannot be transitioned into the required layouts.
   */
  void recordCommandBuffer(uint32_t imageIndex){
    commandBuffers[frameIndex].begin({});  // start recording items into the frame's command buffer

    // Prepare swapchain image so we can draw on it
    transitionImageLayout(
      imageIndex,
      vk::ImageLayout::eUndefined, // discard whatever was in the image previously 
      vk::ImageLayout::eColorAttachmentOptimal, // set image to a render target, able to draw on it now
      {}, // empty access mask, 
      vk::AccessFlagBits2::eColorAttachmentWrite, // set to writing colour data to the image
      vk::PipelineStageFlagBits2::eColorAttachmentOutput, //  treat previous state as if it occurred in the color-attachment stage
      vk::PipelineStageFlagBits2::eColorAttachmentOutput // attachment writes must finish before any future color attachment operations begin
    );

    // Use to clear image to be all black
    vk::ClearValue clearColor = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };

    // Use to specify how to treat the swapchain image during rendering
    vk::RenderingAttachmentInfo attachmentInfo = {
      .imageView = pSwapChain->getImageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear, // clear image first
      .storeOp = vk::AttachmentStoreOp::eStore, // keep the final image when done
      .clearValue = clearColor, // use what was specified above
    };

    // For dynamic rendering
    vk::RenderingInfo renderingInfo {
      .renderArea = {  // rectangle of image to render into
        .offset = { 0, 0 }, 
        .extent = pSwapChain->getExtent(), // render across the whole swapchain image
      },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &attachmentInfo, // as specified above
    };

    
    commandBuffers[frameIndex].beginRendering(renderingInfo);

    // like a "setup" phase for the image before drawing to it
    commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, **pPipeline); // bind the desired graphics pipeline, so the gpu knows how to draw things 
    commandBuffers[frameIndex].bindVertexBuffers(0, *vertexBuffer, {0}); // buffer for the mesh's vertex data
    commandBuffers[frameIndex].bindIndexBuffer( *indexBuffer, 0, vk::IndexType::eUint16 ); // index buffer listing vertex indices used by drawIndexed
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pPipeline->getLayout(), 0, *descriptorSets[frameIndex], nullptr); // uniform buffers, textures, samplers
    // end of "setup" phase

    // determines how clip-space coordinates are mapped to the swapchain image
    // in this case, it will cover the entire image
    commandBuffers[frameIndex].setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(pSwapChain->getExtent().width), 
        static_cast<float>(pSwapChain->getExtent().height), 0.0f, 1.0f));

    // determine where drawing is allowed, currently set to the full image so nothing is clipped
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), pSwapChain->getExtent()));

    // actually invoke drawing, the mesh is rendered here
    commandBuffers[frameIndex].drawIndexed(indices.size(), 1, 0, 0, 0);

    commandBuffers[frameIndex].endRendering();
    
    // Image is finished writing, we can now convert it to somethign that can be shown on screen to the user
    transitionImageLayout(
      imageIndex,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageLayout::ePresentSrcKHR, // swapchain image is ready to be shown on screen
      vk::AccessFlagBits2::eColorAttachmentWrite, // ensure all color writes are finished
      {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
      vk::PipelineStageFlagBits2::eBottomOfPipe // set to wait until the last part of the pipeline is done before we show it to the user
    );

    // stop recording commands into the frame's command buffer
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
  void drawFrame(const sauce::LogicalDevice& logicalDevice){
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
    updateUniformBuffer(frameIndex);

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
  void updateUniformBuffer(uint32_t curImage) {
    // Record the start time on first call (static initialization)
    static auto startTime = std::chrono::high_resolution_clock::now();

    // Get the current time and calculate elapsed time in seconds
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    // Create uniform buffer object with transformation matrices
    sauce::UniformBufferObject ubo {
      // Model matrix: rotates the object 90 degrees per second around the Z axis
      .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
      // View matrix: camera positioned at (2, 2, 2) looking at the origin
      .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.1f)),
      // Projection matrix: 45-degree field of view with 0.1-10.0 clipping plane
      .proj = glm::perspective(glm::radians(45.0f), static_cast<float>(pSwapChain->getExtent().width) / static_cast<float>(pSwapChain->getExtent().height), 0.1f, 10.0f),
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
};

}

