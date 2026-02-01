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
      vk::raii::DeviceMemory& imageMemory
      ) {
    vk::ImageCreateInfo imageInfo{ 
      .imageType = vk::ImageType::e2D,
      .format = format,
      .extent = {width, height, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
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
      vk::ImageAspectFlags aspectFlags
      ) {
    vk::ImageViewCreateInfo viewInfo{ .image = image, .viewType = vk::ImageViewType::e2D,
        .format = format, .subresourceRange = { aspectFlags, 0, 1, 0, 1 } };
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

};

}
