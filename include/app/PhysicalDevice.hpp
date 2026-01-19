#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <app/Instance.hpp>

namespace sauce {

struct PhysicalDevice {

  // ---------------------- constructors ---------------------- 
  PhysicalDevice(std::nullptr_t) {}

  PhysicalDevice(const sauce::Instance& instance) {
    std::vector<vk::raii::PhysicalDevice> devices = (*instance).enumeratePhysicalDevices();

    const auto devIter = std::ranges::find_if(devices, [&](const vk::raii::PhysicalDevice& device) {
      bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
      std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
      bool supportsGraphicsAndPresent = checkQueueFamilySupport(device.getQueueFamilyProperties(), vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eGraphics);
      bool supportsGraphics = std::ranges::any_of(queueFamilies, [](const vk::QueueFamilyProperties& qf) {
          return !!(qf.queueFlags & vk::QueueFlagBits::eGraphics);
      });

      bool supportsAllRequiredDeviceExtensions = checkRequiredExtensions(device, requiredExtensions);
      bool supportsRequiredFeatures = checkRequiredFeatures(device);

      return supportsVulkan1_3 && supportsGraphicsAndPresent && supportsAllRequiredDeviceExtensions && supportsRequiredFeatures;
    });

    if (devIter != devices.end()) {
      physicalDevice = *devIter;
    } else {
      throw std::runtime_error("Failed to find suitable GPU!");
    }
  }

  PhysicalDevice(vk::raii::PhysicalDevice& device) {
    physicalDevice = device;
  }
  //-----------------------------------------------------------

  // checks if a device supports all the device level extensions requested. 
  // ie, hardware ray tracing support. 
  static bool checkRequiredExtensions(const vk::raii::PhysicalDevice& device, const std::vector<const char *> requiredExtensions) {
    auto availableExtensionProps = device.enumerateDeviceExtensionProperties();
    return std::ranges::all_of(requiredExtensions, [&availableExtensionProps](const char* ext) {
      return std::ranges::any_of(availableExtensionProps, [&ext](const vk::ExtensionProperties& extProp){
        return strcmp(ext, extProp.extensionName) == 0;
      });
    });
  }


  // checks for all additional features that are required features for this project
  static bool checkRequiredFeatures(const vk::raii::PhysicalDevice& device) {
    auto availableFeatures = device.getFeatures2<
      vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan11Features,
      vk::PhysicalDeviceVulkan13Features,
      vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >();

    return 
      availableFeatures.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
      availableFeatures.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
      availableFeatures.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
      availableFeatures.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
  }


  // checks if properties of the queue supports the given flags 
  static bool checkQueueFamilySupport(const std::vector<vk::QueueFamilyProperties>& qfProps, vk::QueueFlags mask) {
    return std::ranges::any_of(qfProps, [&mask](const vk::QueueFamilyProperties& qfProp){
      return !!(qfProp.queueFlags & mask);
    });
  }

  // access the underlying PhysicalDevice (*device)
  const vk::raii::PhysicalDevice& operator*() const & noexcept{
    return physicalDevice;
  }

  // access the underlying PhysicalDevice (device->field)
  const vk::raii::PhysicalDevice* operator->() const & noexcept {
    return &physicalDevice;
  }


  // list of extensions used for this project
  std::vector<const char *> requiredExtensions = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
  #ifdef __APPLE__
    "VK_KHR_portability_subset",
  #endif
  };


private:
  vk::raii::PhysicalDevice physicalDevice = nullptr;
};

}
