#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <app/LogicalDevice.hpp>

namespace sauce {

    struct BufferUtils {
        static void createBuffer(const sauce::PhysicalDevice& physicalDevice,
                                 const sauce::LogicalDevice& logicalDevice, vk::DeviceSize size,
                                 vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props,
                                 vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory) {
            vk::BufferCreateInfo bufferCreateInfo{
                .size = size,
                .usage = usage,
                .sharingMode = vk::SharingMode::eExclusive,
            };

            buffer = vk::raii::Buffer{*logicalDevice, bufferCreateInfo};

            vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();

            vk::MemoryAllocateInfo memoryAllocateInfo{
                .allocationSize = memoryRequirements.size,
                .memoryTypeIndex =
                    findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, props),
            };

            bufferMemory = vk::raii::DeviceMemory{*logicalDevice, memoryAllocateInfo};
            buffer.bindMemory(*bufferMemory, 0);
        }

        static uint32_t findMemoryType(const sauce::PhysicalDevice& physicalDevice,
                                       uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
            vk::PhysicalDeviceMemoryProperties deviceMemoryProps =
                physicalDevice->getMemoryProperties();

            for (uint32_t i = 0; i < deviceMemoryProps.memoryTypeCount; ++i) {
                if ((typeFilter & (1 << i)) &&
                    (deviceMemoryProps.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;
            }

            throw std::runtime_error("Failed to find suitable memory type!");
        }

        static void copyBuffer(const sauce::LogicalDevice& logicalDevice,
                               const vk::raii::CommandPool& commandPool,
                               const vk::raii::Queue& queue, vk::raii::Buffer& src,
                               vk::raii::Buffer& dst, vk::DeviceSize size) {
            vk::CommandBufferAllocateInfo copyCommandBufferAllocInfo{
                .commandPool = commandPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1,
            };
            vk::raii::CommandBuffer copyCommandBuffer = std::move(
                logicalDevice->allocateCommandBuffers(copyCommandBufferAllocInfo).front());

            copyCommandBuffer.begin(vk::CommandBufferBeginInfo{
                .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
            copyCommandBuffer.copyBuffer(src, dst, vk::BufferCopy(0, 0, size));
            copyCommandBuffer.end();

            queue.submit(
                vk::SubmitInfo{
                    .commandBufferCount = 1,
                    .pCommandBuffers = &*copyCommandBuffer,
                },
                nullptr);
            queue.waitIdle();
        }
    };

} // namespace sauce
