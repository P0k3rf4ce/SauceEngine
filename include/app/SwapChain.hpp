#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <app/LogicalDevice.hpp>
#include <app/PhysicalDevice.hpp>
#include <app/RenderSurface.hpp>

namespace sauce {

    /**
 * Manages the swap chain: a queue of images waiting to be presented to the screen.
 * The swap chain is essentially Vulkan's frame buffer infrastructure. It holds multiple
 * images that cycle between being rendered to and being displayed.
 */
    struct SwapChain {
        /**
   * Creates a swap chain with sensible settings for what we want to do. We've chosen to optimize
   * for picture smoothness over input latency by using triple buffering (mailbox present mode).
   */
        SwapChain(const sauce::PhysicalDevice& physicalDevice,
                  const sauce::LogicalDevice& logicalDevice,
                  const sauce::RenderSurface& renderSurface, GLFWwindow* window,
                  bool vsync = true) {
            vk::SurfaceCapabilitiesKHR surfaceCapabilities =
                physicalDevice->getSurfaceCapabilitiesKHR(**renderSurface);
            extent = chooseSwapExtent(surfaceCapabilities, window);
            surfaceFormat =
                chooseSwapSurfaceFormat(physicalDevice->getSurfaceFormatsKHR(**renderSurface));

            vk::SwapchainCreateInfoKHR swapChainCreateInfo{
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
                .presentMode = chooseSwapPresentMode(
                    physicalDevice->getSurfacePresentModesKHR(**renderSurface), vsync),
                .clipped = vk::True};

            swapChain = vk::raii::SwapchainKHR{*logicalDevice, swapChainCreateInfo};
            images = swapChain.getImages(); // grab handles to the images vulkan created for us

            // image views describe how to access the image (which part, what format, etc)
            vk::ImageViewCreateInfo imageViewCreateInfo{
                .viewType = vk::ImageViewType::e2D,
                .format = surfaceFormat.format,
                .subresourceRange =
                    {
                        vk::ImageAspectFlagBits::
                            eColor, // we care about color, not depth or stencil
                        0, 1, 0, 1, // base mip level, mip count, base array layer, layer count
                    },
            };

            // create a view for each swap chain image
            for (const auto& image : images) {
                imageViewCreateInfo.image = image;
                imageViews.emplace_back(*logicalDevice, imageViewCreateInfo);
            }
        }

        /**
   * Access the underlying SwapchainKHR.
   */
        const vk::raii::SwapchainKHR& operator*() const& noexcept {
            return swapChain;
        }

        const vk::raii::SwapchainKHR* operator->() const& noexcept {
            return &swapChain;
        }

        /**
   * Returns handles to the swap chain images (the actual render targets).
   */
        const std::vector<vk::Image>& getImages() const noexcept {
            return images;
        }

        /**
   * Returns image views - describes how to interpret each image for rendering.
   */
        const std::vector<vk::raii::ImageView>& getImageViews() const noexcept {
            return imageViews;
        }

        /**
   * Returns the chosen surface format (pixel format + color space).
   */
        const vk::SurfaceFormatKHR& getSurfaceFormat() const noexcept {
            return surfaceFormat;
        }

        /**
   * Returns the swap chain image dimensions in pixels.
   */
        const vk::Extent2D& getExtent() const noexcept {
            return extent;
        }

      private:
        vk::Extent2D extent;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::raii::SwapchainKHR swapChain = nullptr;
        std::vector<vk::Image> images;
        std::vector<vk::raii::ImageView> imageViews;

        /**
   * Selects the best available surface format.
   * Prefers 8-bit BGRA with sRGB color space for accurate color representation.
   */
        static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
            for (const auto& format : availableFormats) {
                // bgra is the most common format, srgb gives us correct gamma
                if (format.format == vk::Format::eB8G8R8A8Srgb &&
                    format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                    return format;
            }

            return availableFormats[0]; // fallback to whatever's available
        }

        /**
   * Selects the presentation mode based on the vsync setting.
   * When vsync is true, returns FIFO (vertical sync, guaranteed available).
   * When vsync is false, prefers Mailbox (triple buffering), then Immediate
   * (uncapped framerate), falling back to FIFO if neither is available.
   */
        static vk::PresentModeKHR chooseSwapPresentMode(
            const std::vector<vk::PresentModeKHR>& availablePresentModes, bool vsync = true) {
            if (vsync) {
                return vk::PresentModeKHR::eFifo;
            }

            for (const auto& mode : availablePresentModes) {
                if (mode == vk::PresentModeKHR::eMailbox) {
                    return mode;
                }
            }

            for (const auto& mode : availablePresentModes) {
                if (mode == vk::PresentModeKHR::eImmediate) {
                    return mode;
                }
            }

            return vk::PresentModeKHR::eFifo;
        }

        /**
   * Determines the swap chain image resolution
   * This should usually match window size, clamped to the surface's supported range
   * High-DPI (like apple retina) displays may have different framebuffer vs window coordinates
   */
        static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                                             GLFWwindow* window) {
            // if not max uint32, window manager already set the extent for us
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
                return capabilities.currentExtent;

            // query actual framebuffer size
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            return {
                std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
                                     capabilities.maxImageExtent.width),
                std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height),
            };
        }

        /**
   * Determines how many images the swap chain should hold
   * Requests at least 3 images (triple buffering) when possible
   */
        static uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& capabilities) {
            auto minImageCount = std::max(3u, capabilities.minImageCount);
            if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < minImageCount)) {
                minImageCount = capabilities.maxImageCount;
            }
            return minImageCount;
        }
    };

} // namespace sauce
