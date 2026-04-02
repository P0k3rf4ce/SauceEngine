#pragma once

#include "app/Vertex.hpp"
#include "app/modeling/PropertyValue.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan_raii.hpp>

namespace sauce {
struct LogicalDevice;
namespace modeling {

class Mesh {
public:
    Mesh() = default;
    Mesh(const std::vector<sauce::Vertex>& vertices,
         const std::vector<uint32_t>& indices);

    const std::vector<sauce::Vertex>& getVertices() const { return vertices; }
    std::vector<sauce::Vertex>& getVerticesMutable() { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }

    size_t getVertexCount() const { return vertices.size(); }
    size_t getIndexCount() const { return indices.size(); }

    bool hasGPUData() const { return vertexBuffer != nullptr; }

    bool isValid() const;

    void initVulkanResources(
        vk::raii::Device& device,
        vk::raii::PhysicalDevice& physicalDevice);

    void bind(vk::raii::CommandBuffer& cmd);

    void draw(vk::raii::CommandBuffer& cmd);

    void setPipelineLayout(vk::raii::PipelineLayout* layout) {
        pipelineLayout = layout;
    }

    vk::PipelineLayout getPipelineLayout() const {
        return pipelineLayout ? **pipelineLayout : VK_NULL_HANDLE;
    }

    const std::unordered_map<std::string, PropertyValue>& getMetadata() const { return metadata; }
    void setMetadata(const std::string& key, const PropertyValue& value);
    bool hasMetadata(const std::string& key) const;

    void generateNormals();
    void generateTangents();

    // Optional GPU upload (Phase 6)
    void initVulkanResources(const sauce::LogicalDevice& logicalDevice, vk::raii::PhysicalDevice& physicalDevice, vk::raii::CommandPool& commandPool, vk::raii::Queue& queue);

private:
    // CPU data
    std::vector<sauce::Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<std::string, PropertyValue> metadata;

    // GPU resources (optional, for Phase 6)
    std::unique_ptr<vk::raii::Buffer> vertexBuffer;
    std::unique_ptr<vk::raii::Buffer> indexBuffer;
    std::unique_ptr<vk::raii::DeviceMemory> vertexBufferMemory;
    std::unique_ptr<vk::raii::DeviceMemory> indexBufferMemory;

    vk::raii::PipelineLayout* pipelineLayout = nullptr;

    uint32_t findMemoryType(
        vk::PhysicalDeviceMemoryProperties memProperties,
        uint32_t typeFilter,
        vk::MemoryPropertyFlags properties);
};

} // namespace modeling
} // namespace sauce
