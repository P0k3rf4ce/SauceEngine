#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>

namespace sauce {

struct Instance {
  Instance(const char** glfwExtensions, uint32_t glfwExtensionCount) {
    constexpr vk::ApplicationInfo appInfo {
      .pApplicationName = "Vulkan Playground",
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = vk::ApiVersion14
    };

    auto extensions = getRequiredExtensions(glfwExtensions, glfwExtensionCount);
    checkRequiredExtensions(extensions);

    checkRequiredLayers(validationLayers);
    
   vk::InstanceCreateInfo createInfo {
#ifdef __APPLE__
      .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif 
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
      .ppEnabledLayerNames = validationLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
    };

    instance = { context, createInfo };

    setupDebugMessenger();
  }

  const vk::raii::Instance& operator*() const & noexcept {
    return instance;
  }

  const vk::raii::Instance* operator->() const & noexcept {
    return &instance;
  }

private:
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

#ifdef NDEBUG
  constexpr static bool enableValidationLayers = false;
#else
  constexpr static bool enableValidationLayers = true;
#endif

  const std::vector<char const *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  static std::vector<const char *> getRequiredExtensions(const char** glfwExtensions, uint32_t glfwExtensionCount) {
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
      extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

#ifdef __APPLE__
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

    return extensions;
  }

  void checkRequiredExtensions(const std::vector<const char *>& requiredExtensions) {
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (size_t i = 0; i < requiredExtensions.size(); ++i) {
      if (std::ranges::none_of(extensionProperties.begin(), extensionProperties.end(),
            [&extension = requiredExtensions[i]](auto const& extensionProperty) {
              return strcmp(extensionProperty.extensionName, extension) == 0;
            }
          ))
      {
        throw std::runtime_error("Required extension not supported: " + std::string(requiredExtensions[i]));
      }
    }
  }

  void checkRequiredLayers(const std::vector<const char *> requiredLayers) {
    auto layerProps = context.enumerateInstanceLayerProperties();
    for (size_t i = 0; i < requiredLayers.size(); ++i) {
      if (std::ranges::none_of(layerProps.begin(), layerProps.end(),
            [&requiredLayer = requiredLayers[i]](auto const& layerProp) {
              return strcmp(requiredLayer, layerProp.layerName) == 0;
            }
          ))
      {
        throw std::runtime_error("Required layer not supported: " + std::string(requiredLayers[i]));
      }
    }
  }

  void setupDebugMessenger() {
    if (!enableValidationLayers) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags { vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError };
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags { vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation };

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT {
      .messageSeverity = severityFlags,
      .messageType = messageTypeFlags,
      .pfnUserCallback = &debugCallback,
    };

    debugMessenger = { instance, debugUtilsMessengerCreateInfoEXT };
  }

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
      void*
  ) {
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
      std::cerr << "Validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
    }

    return vk::False;
  }
};

}
