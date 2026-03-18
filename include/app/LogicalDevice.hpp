#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <app/PhysicalDevice.hpp>
#include <app/RenderSurface.hpp>

namespace sauce {

struct LogicalDevice {

  LogicalDevice(std::nullptr_t) {}

  LogicalDevice(const sauce::PhysicalDevice& physicalDevice, const sauce::RenderSurface& surface) {
    std::vector<vk::QueueFamilyProperties> queueFamilyProps = (*physicalDevice).getQueueFamilyProperties();

    auto graphicsQueueFamilyProp = std::ranges::find_if(queueFamilyProps, [&](const vk::QueueFamilyProperties& prop){
        return (prop.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
    });

    assert(graphicsQueueFamilyProp != queueFamilyProps.end());

    queueIndex = static_cast<uint32_t>(std::distance(queueFamilyProps.begin(), graphicsQueueFamilyProp));

    
    if (!(*physicalDevice).getSurfaceSupportKHR(queueIndex, **surface)) { 
      throw std::runtime_error("Presentation not supported in the chosen queue!");
    }

    vk::StructureChain<
      vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan11Features,
      vk::PhysicalDeviceVulkan13Features, 
      vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > featureChain 
    {
      {.features = {.samplerAnisotropy = true }},
      { .shaderDrawParameters = true },
      { .synchronization2 = true, .dynamicRendering = true, },
      { .extendedDynamicState = true },
    };

    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo {
      .queueFamilyIndex = queueIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority,
    };
    vk::DeviceCreateInfo deviceCreateInfo {
      .pNext = &featureChain.get(),
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledExtensionCount = static_cast<uint32_t>(physicalDevice.requiredExtensions.size()),
      .ppEnabledExtensionNames = physicalDevice.requiredExtensions.data(),
    };

    device = vk::raii::Device { *physicalDevice, deviceCreateInfo };
  }

  const vk::raii::Device& operator*() const & noexcept {
    return device;
  }

  const vk::raii::Device* operator->() const & noexcept {
    return &device;
  }

  uint32_t getQueueIndex() const noexcept {
    return queueIndex;
  }

private:
  vk::raii::Device device = nullptr;
  uint32_t queueIndex = ~0;
};

}
