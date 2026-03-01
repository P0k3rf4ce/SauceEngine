#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include <app/LogicalDevice.hpp>
#include <app/PhysicalDevice.hpp>

namespace sauce {

struct IBLMaps {
    vk::raii::Image envCubemap = nullptr;
    vk::raii::DeviceMemory envCubemapMemory = nullptr;
    vk::raii::ImageView envCubemapView = nullptr;

    vk::raii::Image irradianceMap = nullptr;
    vk::raii::DeviceMemory irradianceMapMemory = nullptr;
    vk::raii::ImageView irradianceMapView = nullptr;

    vk::raii::Image prefilterMap = nullptr;
    vk::raii::DeviceMemory prefilterMapMemory = nullptr;
    vk::raii::ImageView prefilterMapView = nullptr;

    vk::raii::Image brdfLUT = nullptr;
    vk::raii::DeviceMemory brdfLUTMemory = nullptr;
    vk::raii::ImageView brdfLUTView = nullptr;

    vk::raii::Sampler sampler = nullptr;
};

class IBLGenerator {
public:
    IBLGenerator(const sauce::PhysicalDevice& physicalDevice, const sauce::LogicalDevice& logicalDevice);

    std::unique_ptr<IBLMaps> generateIBLMaps(
        const std::string& hdrPath,
        const vk::raii::CommandPool& commandPool,
        const vk::raii::Queue& queue
    );

private:
    const sauce::PhysicalDevice& physicalDevice;
    const sauce::LogicalDevice& logicalDevice;

    void convertEquirectangularToCubemap(
        const vk::raii::ImageView& hdrView,
        IBLMaps& maps,
        const vk::raii::CommandPool& commandPool,
        const vk::raii::Queue& queue
    );

    void generateIrradianceMap(
        IBLMaps& maps,
        const vk::raii::CommandPool& commandPool,
        const vk::raii::Queue& queue
    );

    void generatePrefilterMap(
        IBLMaps& maps,
        const vk::raii::CommandPool& commandPool,
        const vk::raii::Queue& queue
    );

    void generateBRDFLUT(
        IBLMaps& maps,
        const vk::raii::CommandPool& commandPool,
        const vk::raii::Queue& queue
    );

    // Helpers
    vk::raii::ShaderModule createShaderModule(const std::string& filename);
    void createCubeMesh(const vk::raii::CommandPool& commandPool, const vk::raii::Queue& queue);

    vk::raii::Buffer vertexBuffer = nullptr;
    vk::raii::DeviceMemory vertexBufferMemory = nullptr;
    vk::raii::Buffer indexBuffer = nullptr;
    vk::raii::DeviceMemory indexBufferMemory = nullptr;
    uint32_t indexCount = 0;
};

} // namespace sauce
