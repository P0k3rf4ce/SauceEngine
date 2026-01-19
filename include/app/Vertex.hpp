#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

namespace sauce {

/**
 * @brief Vertex layout used by the graphics pipeline.
 *
 * Position is a 2D vector in clip space, color is RGB in linear space.
 */
struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  /**
   * @brief Returns the Vulkan binding description for a single Vertex buffer.
   */
  static vk::VertexInputBindingDescription getBindingDescription() {
    return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
  }

  /**
   * @brief Returns the Vulkan attribute descriptions for position and color.
   */
  static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription() {
    return {
      vk::VertexInputAttributeDescription { 0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos) },
      vk::VertexInputAttributeDescription { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) },
    };
  }
};

/**
 * @brief Per-frame uniform data passed to the vertex shader.
 *
 * Matrices are 16-byte aligned to match Vulkan std140 layout rules.
 */
struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

}

