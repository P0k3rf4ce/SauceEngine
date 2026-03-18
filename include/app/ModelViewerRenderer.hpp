#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <chrono>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/BufferUtils.hpp>
#include <app/Camera.hpp>
#include <app/GraphicsPipeline.hpp>
#include <app/ImGuiRenderer.hpp>
#include <app/LogicalDevice.hpp>
#include <app/SwapChain.hpp>
#include <app/modeling/GLTFLoader.hpp>
#include <app/modeling/Model.hpp>

namespace sauce {

    struct MeshGPUData {
        vk::raii::Buffer vertexBuffer = nullptr;
        vk::raii::DeviceMemory vertexBufferMemory = nullptr;
        vk::raii::Buffer indexBuffer = nullptr;
        vk::raii::DeviceMemory indexBufferMemory = nullptr;
        uint32_t indexCount = 0;
    };

    struct RendererCreateInfo;

    class ModelViewerRenderer {
      public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        ModelViewerRenderer(const RendererCreateInfo& createInfo);

        void loadModel(const std::string& modelPath, const sauce::PhysicalDevice& physicalDevice,
                       const sauce::LogicalDevice& logicalDevice);

        void drawFrame(const sauce::LogicalDevice& logicalDevice, const sauce::Camera& camera,
                       float modelRotationAngle, sauce::ImGuiRenderer* imguiRenderer = nullptr);

        const vk::raii::Queue& getQueue() const {
            return *pQueue;
        }
        const sauce::SwapChain& getSwapChain() const {
            return *pSwapChain;
        }
        const vk::raii::CommandPool& getCommandPool() const {
            return commandPool;
        }

      private:
        std::vector<MeshGPUData> meshBuffers;
        std::shared_ptr<sauce::modeling::Model> pModel;

        void uploadMeshToGPU(const std::shared_ptr<sauce::modeling::Mesh>& mesh,
                             const sauce::PhysicalDevice& physicalDevice,
                             const sauce::LogicalDevice& logicalDevice);

        void createDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice);
        void createDescriptorSets(const sauce::LogicalDevice& logicalDevice);
        void createUniformBuffers(const sauce::PhysicalDevice& physicalDevice,
                                  const sauce::LogicalDevice& logicalDevice);

        void transitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout,
                                   vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
                                   vk::AccessFlags2 dstAccessMask,
                                   vk::PipelineStageFlags2 srcStageMask,
                                   vk::PipelineStageFlags2 dstStageMask);

        void recordCommandBuffer(uint32_t imageIndex, sauce::ImGuiRenderer* imguiRenderer);

        void updateUniformBuffer(uint32_t curImage, const sauce::Camera& camera,
                                 float modelRotationAngle);

        std::unique_ptr<vk::raii::Queue> pQueue;
        std::unique_ptr<sauce::SwapChain> pSwapChain;

        vk::raii::CommandPool commandPool = nullptr;
        std::vector<vk::raii::CommandBuffer> commandBuffers;

        std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
        std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
        std::vector<vk::raii::Fence> inFlightFences;

        uint32_t frameIndex = 0;
        uint32_t queueIndex = ~0;

        std::unique_ptr<sauce::GraphicsPipeline> pPipeline;

        vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
        vk::raii::DescriptorPool descriptorPool = nullptr;
        std::vector<vk::raii::DescriptorSet> descriptorSets;

        std::vector<vk::raii::Buffer> uniformBuffers;
        std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;
    };

} // namespace sauce
