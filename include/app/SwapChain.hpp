#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <app/LogicalDevice.hpp>
#include <app/PhysicalDevice.hpp>
#include <app/RenderSurface.hpp>

namespace sauce {

struct SwapChain {

  SwapChain(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      const sauce::RenderSurface& renderSurface, 
      GLFWwindow* window
  ) {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice->getSurfaceCapabilitiesKHR(**renderSurface);
    extent = chooseSwapExtent(surfaceCapabilities, window);
    surfaceFormat = chooseSwapSurfaceFormat(physicalDevice->getSurfaceFormatsKHR(**renderSurface));
    vk::SwapchainCreateInfoKHR swapChainCreateInfo {
      .surface = **renderSurface,
      .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = chooseSwapPresentMode(physicalDevice->getSurfacePresentModesKHR(**renderSurface)),
      .clipped = vk::True
    };

    swapChain = vk::raii::SwapchainKHR { *logicalDevice, swapChainCreateInfo };
    images = swapChain.getImages();

    vk::ImageViewCreateInfo imageViewCreateInfo {
      .viewType = vk::ImageViewType::e2D,
      .format = surfaceFormat.format,
      .subresourceRange = {
        vk::ImageAspectFlagBits::eColor,
        0, 1, 0, 1,
      },
    };

    for (const auto& image: images) {
      imageViewCreateInfo.image = image;
      imageViews.emplace_back( *logicalDevice, imageViewCreateInfo );
    }
  }

  const vk::raii::SwapchainKHR& operator*() const & noexcept {
    return swapChain;
  }

  const vk::raii::SwapchainKHR* operator->() const & noexcept {
    return &swapChain;
  }

  const std::vector<vk::Image>& getImages() const noexcept {
    return images;
  }

  const std::vector<vk::raii::ImageView>& getImageViews() const noexcept {
    return imageViews;
  }

  const vk::SurfaceFormatKHR& getSurfaceFormat() const noexcept {
    return surfaceFormat;
  }

  const vk::Extent2D& getExtent() const noexcept {
    return extent;
  }

private:
  vk::Extent2D extent;
  vk::SurfaceFormatKHR surfaceFormat;
  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> images;
  std::vector<vk::raii::ImageView> imageViews;

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

  static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
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

};

}
