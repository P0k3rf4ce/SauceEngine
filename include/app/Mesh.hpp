#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <app/Vertex.hpp>
#include <app/BufferUtils.hpp>

#include <vector>
#include <cstring>

namespace sauce {

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  vk::raii::Buffer vertexBuffer = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory = nullptr;
  vk::raii::Buffer indexBuffer = nullptr;
  vk::raii::DeviceMemory indexBufferMemory = nullptr;

  Mesh() = default;

  Mesh(std::vector<Vertex> verts, std::vector<uint32_t> inds)
      : vertices(std::move(verts)), indices(std::move(inds)) {}

  void uploadToGPU(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::CommandPool& commandPool,
      const vk::raii::Queue queue
  ) {
    createVertexBuffer(physicalDevice, logicalDevice, commandPool, queue);
    createIndexBuffer(physicalDevice, logicalDevice, commandPool, queue);
  }

  uint32_t getIndexCount() const { return static_cast<uint32_t>(indices.size()); }
  uint32_t getVertexCount() const { return static_cast<uint32_t>(vertices.size()); }

private:
  void createVertexBuffer(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::CommandPool& commandPool,
      const vk::raii::Queue queue
  ) {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;

    BufferUtils::createBuffer(
        physicalDevice, logicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingBufferMemory
    );

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    stagingBufferMemory.unmapMemory();

    BufferUtils::createBuffer(
        physicalDevice, logicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer, vertexBufferMemory
    );

    BufferUtils::copyBuffer(logicalDevice, commandPool, queue, stagingBuffer, vertexBuffer, bufferSize);
  }

  void createIndexBuffer(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::CommandPool& commandPool,
      const vk::raii::Queue queue
  ) {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;

    BufferUtils::createBuffer(
        physicalDevice, logicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingBufferMemory
    );

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    stagingBufferMemory.unmapMemory();

    BufferUtils::createBuffer(
        physicalDevice, logicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        indexBuffer, indexBufferMemory
    );

    BufferUtils::copyBuffer(logicalDevice, commandPool, queue, stagingBuffer, indexBuffer, bufferSize);
  }
};

}
