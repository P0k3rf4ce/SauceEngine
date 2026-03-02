#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <app/LogicalDevice.hpp>

namespace sauce {

struct ImageUtils {
  static void createImage(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      uint32_t width, 
      uint32_t height, 
      vk::Format format, 
      vk::ImageTiling tiling, 
      vk::ImageUsageFlags usage, 
      vk::MemoryPropertyFlags properties, 
      vk::raii::Image& image, 
      vk::raii::DeviceMemory& imageMemory,
      uint32_t mipLevels = 1,
      uint32_t arrayLayers = 1,
      vk::ImageCreateFlags flags = {}
      ) {
    vk::ImageCreateInfo imageInfo{ 
      .flags = flags,
      .imageType = vk::ImageType::e2D,
      .format = format,
      .extent = {width, height, 1},
      .mipLevels = mipLevels,
      .arrayLayers = arrayLayers,
      .samples = vk::SampleCountFlagBits::e1,
      .tiling = tiling,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
    };

    image = vk::raii::Image(*logicalDevice, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{ 
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) 
    };
    imageMemory = vk::raii::DeviceMemory(*logicalDevice, allocInfo);
    image.bindMemory(imageMemory, 0);
  }

  static vk::raii::ImageView createImageView(
      const sauce::LogicalDevice& logicalDevice,
      vk::raii::Image& image, vk::Format format, 
      vk::ImageAspectFlags aspectFlags,
      vk::ImageViewType viewType = vk::ImageViewType::e2D,
      uint32_t mipLevels = 1,
      uint32_t arrayLayers = 1,
      uint32_t baseMipLevel = 0,
      uint32_t baseArrayLayer = 0
      ) {
    vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = viewType,
        .format = format, .subresourceRange = { aspectFlags, baseMipLevel, mipLevels, baseArrayLayer, arrayLayers } };
    return vk::raii::ImageView { *logicalDevice, viewInfo };
  }

  static uint32_t findMemoryType(const sauce::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties deviceMemoryProps = physicalDevice->getMemoryProperties();

    for (uint32_t i = 0; i < deviceMemoryProps.memoryTypeCount; ++i) {
      if ((typeFilter & (1 << i)) && (deviceMemoryProps.memoryTypes[i].propertyFlags & properties) == properties)
        return i;
    }

    throw std::runtime_error("Failed to find suitable memory type!");
  }

  static void copyBufferToImage(
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::CommandPool& commandPool,
      const vk::raii::Queue& queue,
      const vk::raii::Buffer& srcBuffer,
      vk::raii::Image& dstImage,
      uint32_t width,
      uint32_t height
  ) {
    vk::CommandBufferAllocateInfo copyCommandBufferAllocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
    };
    vk::raii::CommandBuffer copyCommandBuffer = std::move(logicalDevice->allocateCommandBuffers(copyCommandBufferAllocInfo).front());

    copyCommandBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    vk::BufferImageCopy region {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
      .imageOffset = {0, 0, 0},
      .imageExtent = {width, height, 1},
    };

    copyCommandBuffer.copyBufferToImage(srcBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, region);
    copyCommandBuffer.end();

    queue.submit(vk::SubmitInfo{
      .commandBufferCount = 1,
      .pCommandBuffers = &*copyCommandBuffer,
    }, nullptr);
    queue.waitIdle();
  }

  static void transitionImageLayout(
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::CommandPool& commandPool,
      const vk::raii::Queue& queue,
      vk::raii::Image& image,
      vk::ImageLayout oldLayout,
      vk::ImageLayout newLayout,
      vk::AccessFlags2 srcAccessMask,
      vk::AccessFlags2 dstAccessMask,
      vk::PipelineStageFlags2 srcStageMask,
      vk::PipelineStageFlags2 dstStageMask,
      uint32_t mipLevels = 1,
      uint32_t arrayLayers = 1,
      uint32_t baseMipLevel = 0,
      uint32_t baseArrayLayer = 0
  ) {
    vk::CommandBufferAllocateInfo transitionCommandBufferAllocInfo {
      .commandPool = commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
    };
    vk::raii::CommandBuffer transitionCommandBuffer = std::move(logicalDevice->allocateCommandBuffers(transitionCommandBufferAllocInfo).front());

    transitionCommandBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    vk::ImageMemoryBarrier2 barrier {
      .srcStageMask = srcStageMask,
      .srcAccessMask = srcAccessMask,
      .dstStageMask = dstStageMask,
      .dstAccessMask = dstAccessMask,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = baseMipLevel,
        .levelCount = mipLevels,
        .baseArrayLayer = baseArrayLayer,
        .layerCount = arrayLayers,
      },
    };

    vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &barrier,
    };

    transitionCommandBuffer.pipelineBarrier2(dependencyInfo);
    transitionCommandBuffer.end();

    queue.submit(vk::SubmitInfo{
      .commandBufferCount = 1,
      .pCommandBuffers = &*transitionCommandBuffer,
    }, nullptr);
    queue.waitIdle();
  }

};

}
