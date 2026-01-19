#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <app/LogicalDevice.hpp>

namespace sauce {

/**
 * @brief Helper utilities for creating and copying Vulkan buffers.
 */
struct BufferUtils {

  /**
   * @brief Creates a buffer and allocates/binds device memory for it.
   *
   * @param physicalDevice The GPU used to query memory properties.
   * @param logicalDevice The device used to create buffers and allocate memory.
   * @param size Size of the buffer in bytes.
   * @param usage Buffer usage flags (vertex, index, uniform, transfer, etc.).
   * @param props Required memory properties (device-local, host-visible, etc.).
   * @param buffer Output buffer handle.
   * @param bufferMemory Output memory handle bound to the buffer.
   *
   * @throws std::runtime_error If a compatible memory type is not found.
   */
  static void createBuffer(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      vk::DeviceSize size, 
      vk::BufferUsageFlags usage, 
      vk::MemoryPropertyFlags props, 
      vk::raii::Buffer& buffer, 
      vk::raii::DeviceMemory& bufferMemory
  ) {
    vk::BufferCreateInfo bufferCreateInfo {
      .size = size,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
    };

    buffer = vk::raii::Buffer { *logicalDevice, bufferCreateInfo };

    vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo memoryAllocateInfo {
      .allocationSize = memoryRequirements.size,
      .memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, props), 
    };

    bufferMemory = vk::raii::DeviceMemory{ *logicalDevice, memoryAllocateInfo };
    buffer.bindMemory(*bufferMemory, 0);
  }

  /**
   * @brief Finds a GPU memory type that satisfies a bitmask and property flags.
   *
   * @param physicalDevice The GPU used to query memory properties.
   * @param typeFilter Bitmask of acceptable memory types.
   * @param properties Required memory property flags.
   * @return Index of a suitable memory type.
   *
   * @throws std::runtime_error If no compatible memory type exists.
   */
  static uint32_t findMemoryType(
      const sauce::PhysicalDevice& physicalDevice,
      uint32_t typeFilter, 
      vk::MemoryPropertyFlags properties
  ) {
    vk::PhysicalDeviceMemoryProperties deviceMemoryProps = physicalDevice->getMemoryProperties();

    for (uint32_t i = 0; i < deviceMemoryProps.memoryTypeCount; ++i) {
      if ((typeFilter & (1 << i)) && (deviceMemoryProps.memoryTypes[i].propertyFlags & properties) == properties)
        return i;
    }

    throw std::runtime_error("Failed to find suitable memory type!");
  }

  /**
   * @brief Copies data from one buffer to another using a one-time command buffer.
   *
   * @param logicalDevice The device used to allocate and submit the command buffer.
   * @param commandPool Command pool used to allocate the copy command buffer.
   * @param queue Queue that executes the copy operation.
   * @param src Source buffer.
   * @param dst Destination buffer.
   * @param size Size of the copy in bytes.
   */
  static void copyBuffer(
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::CommandPool& commandPool,
      const vk::raii::Queue queue,
      vk::raii::Buffer& src, 
      vk::raii::Buffer& dst, 
      vk::DeviceSize size
  ) {
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

};

}
