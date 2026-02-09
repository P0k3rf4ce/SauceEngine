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
    Mesh();
    Mesh(const std::vector<sauce::Vertex>& vertices, const std::vector<uint32_t>& indices);

    // Getters
    const std::vector<sauce::Vertex>& getVertices() const { return vertices; }
    std::vector<sauce::Vertex>& getVertices() { return vertices; }

    const std::vector<uint32_t>& getIndices() const { return indices; }
    std::vector<uint32_t>& getIndices() { return indices; }

    size_t getVertexCount() const { return vertices.size(); }
    size_t getIndexCount() const { return indices.size(); }

    // GPU status
    bool hasGPUData() const { return vertexBuffer != nullptr; }

    // Validation
    bool isValid() const;

    // Metadata access (for GLTF extensions)
    const std::unordered_map<std::string, PropertyValue>& getMetadata() const { return metadata; }
    void setMetadata(const std::string& key, const PropertyValue& value);
    bool hasMetadata(const std::string& key) const;

    // Utility methods for missing vertex attributes
    void generateNormals();
    void generateTangents();

    // Optional GPU upload (Phase 6)
    void initVulkanResources(const sauce::LogicalDevice& logicalDevice, vk::raii::PhysicalDevice& physicalDevice, vk::raii::CommandPool& commandPool, vk::raii::Queue& queue);
    void bind(vk::raii::CommandBuffer& commandBuffer);
    void draw(vk::raii::CommandBuffer& commandBuffer);

private:
    std::vector<sauce::Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<std::string, PropertyValue> metadata;

    // GPU resources (optional, for Phase 6)
    std::unique_ptr<vk::raii::Buffer> vertexBuffer;
    std::unique_ptr<vk::raii::Buffer> indexBuffer;
    std::unique_ptr<vk::raii::DeviceMemory> vertexBufferMemory;
    std::unique_ptr<vk::raii::DeviceMemory> indexBufferMemory;
};

} // namespace modeling
} // namespace sauce
