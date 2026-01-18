#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

namespace sauce {

struct Vertex {
  glm::vec3 position;
  glm::vec2 uv;
  glm::vec3 normal;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
  }

  static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescription() {
    return {
      vk::VertexInputAttributeDescription { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) },
      vk::VertexInputAttributeDescription { 1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv) },
      vk::VertexInputAttributeDescription { 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) },
    };
  }

  bool operator==(const Vertex& other) const {
    return position == other.position && uv == other.uv && normal == other.normal;
  }
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

}

