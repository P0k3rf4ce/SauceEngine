#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

namespace sauce {

    struct Vertex {
        glm::vec3 position;  // 3D position
        glm::vec3 normal;    // Surface normal
        glm::vec2 texCoords; // UV coordinates
        glm::vec3 color;     // Vertex color
        glm::vec4 tangent;   // Tangent (w = handedness)

        static vk::VertexInputBindingDescription getBindingDescription() {
            return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
        }

        static std::array<vk::VertexInputAttributeDescription, 5> getAttributeDescription() {
            return {
                vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat,
                                                    offsetof(Vertex, position)},
                vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat,
                                                    offsetof(Vertex, normal)},
                vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat,
                                                    offsetof(Vertex, texCoords)},
                vk::VertexInputAttributeDescription{3, 0, vk::Format::eR32G32B32Sfloat,
                                                    offsetof(Vertex, color)},
                vk::VertexInputAttributeDescription{4, 0, vk::Format::eR32G32B32A32Sfloat,
                                                    offsetof(Vertex, tangent)},
            };
        }
    };

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec3 cameraPos;
    };

} // namespace sauce
