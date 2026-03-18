#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <app/GraphicsPipeline.hpp>
#include <app/ImageUtils.hpp>

#include <imgui_impl_vulkan.h>

namespace sauce::editor {

    class OffscreenFramebuffer {
      public:
        static constexpr vk::Format COLOR_FORMAT = vk::Format::eR8G8B8A8Unorm;

        OffscreenFramebuffer(const sauce::PhysicalDevice& physicalDevice,
                             const sauce::LogicalDevice& logicalDevice, uint32_t initialWidth,
                             uint32_t initialHeight)
            : pPhysicalDevice(&physicalDevice), pLogicalDevice(&logicalDevice) {
            // Create sampler for the offscreen color image
            vk::SamplerCreateInfo samplerInfo{
                .magFilter = vk::Filter::eLinear,
                .minFilter = vk::Filter::eLinear,
                .mipmapMode = vk::SamplerMipmapMode::eLinear,
                .addressModeU = vk::SamplerAddressMode::eClampToEdge,
                .addressModeV = vk::SamplerAddressMode::eClampToEdge,
                .addressModeW = vk::SamplerAddressMode::eClampToEdge,
                .mipLodBias = 0.0f,
                .anisotropyEnable = vk::False,
                .compareEnable = vk::False,
                .minLod = 0.0f,
                .maxLod = 1.0f,
                .borderColor = vk::BorderColor::eFloatOpaqueBlack,
            };
            sampler = vk::raii::Sampler{*logicalDevice, samplerInfo};

            resize(initialWidth, initialHeight);
        }

        void resize(uint32_t newWidth, uint32_t newHeight) {
            if (newWidth == 0 || newHeight == 0)
                return;
            if (newWidth == width && newHeight == height && colorImage != nullptr)
                return;

            width = newWidth;
            height = newHeight;

            // Remove old ImGui descriptor if it exists
            if (imguiDescriptorSet != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RemoveTexture(imguiDescriptorSet);
                imguiDescriptorSet = VK_NULL_HANDLE;
            }

            // Recreate color image
            ImageUtils::createImage(
                *pPhysicalDevice, *pLogicalDevice, width, height, COLOR_FORMAT,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal, colorImage, colorImageMemory);
            colorImageView = ImageUtils::createImageView(*pLogicalDevice, colorImage, COLOR_FORMAT,
                                                         vk::ImageAspectFlagBits::eColor);

            // Recreate depth image
            vk::Format depthFormat = GraphicsPipeline::findDepthFormat(*pPhysicalDevice);
            ImageUtils::createImage(
                *pPhysicalDevice, *pLogicalDevice, width, height, depthFormat,
                vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
            depthImageView = ImageUtils::createImageView(*pLogicalDevice, depthImage, depthFormat,
                                                         vk::ImageAspectFlagBits::eDepth);

            // Create ImGui descriptor set for displaying the offscreen texture
            imguiDescriptorSet = ImGui_ImplVulkan_AddTexture(
                *sampler, *colorImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        VkDescriptorSet getImGuiDescriptorSet() const {
            return imguiDescriptorSet;
        }

        const vk::raii::Image& getColorImage() const {
            return colorImage;
        }
        const vk::raii::ImageView& getColorImageView() const {
            return colorImageView;
        }
        const vk::raii::Image& getDepthImage() const {
            return depthImage;
        }
        const vk::raii::ImageView& getDepthImageView() const {
            return depthImageView;
        }

        uint32_t getWidth() const {
            return width;
        }
        uint32_t getHeight() const {
            return height;
        }

        ~OffscreenFramebuffer() {
            if (imguiDescriptorSet != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RemoveTexture(imguiDescriptorSet);
                imguiDescriptorSet = VK_NULL_HANDLE;
            }
        }

        // Non-copyable
        OffscreenFramebuffer(const OffscreenFramebuffer&) = delete;
        OffscreenFramebuffer& operator=(const OffscreenFramebuffer&) = delete;

      private:
        const sauce::PhysicalDevice* pPhysicalDevice;
        const sauce::LogicalDevice* pLogicalDevice;

        uint32_t width = 0;
        uint32_t height = 0;

        vk::raii::Image colorImage = nullptr;
        vk::raii::DeviceMemory colorImageMemory = nullptr;
        vk::raii::ImageView colorImageView = nullptr;

        vk::raii::Image depthImage = nullptr;
        vk::raii::DeviceMemory depthImageMemory = nullptr;
        vk::raii::ImageView depthImageView = nullptr;

        vk::raii::Sampler sampler = nullptr;

        VkDescriptorSet imguiDescriptorSet = VK_NULL_HANDLE;
    };

} // namespace sauce::editor
