#pragma once

#include "app/components/LightComponent.hpp"
#include <glm/glm.hpp>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace sauce {
    struct LogicalDevice;
    struct PhysicalDevice;
}

namespace sauce {

struct alignas(16) GPUSpotLight {
    glm::vec3 position{0.0f};          // offset  0
    float     intensity{1.0f};         // offset 12
    glm::vec3 direction{0.0f, -1.0f, 0.0f}; // offset 16
    float     innerConeAngle{0.0f};    // offset 28
    glm::vec3 color{1.0f};             // offset 32
    float     outerConeAngle{0.7853981f}; // offset 44
    float     range{0.0f};             // offset 48
    float     _pad0[3]{};              // offset 52
};
static_assert(sizeof(GPUSpotLight) == 64, "GPUSpotLight must be 64 bytes to match shader_lights.slang");

class SpotLightComponent : public LightComponent {
public:
    SpotLightComponent();
    SpotLightComponent(const glm::vec3& color, float intensity, float range = 0.0f,
                       const glm::vec3& direction = glm::vec3(0.0f, -1.0f, 0.0f),
                       float innerConeAngle = 0.0f,
                       float outerConeAngle = 0.7853981f);
    ~SpotLightComponent() override = default;

    float getRange() const { return range; }
    void  setRange(float r) { range = r; }

    const glm::vec3& getDirection() const { return direction; }
    void setDirection(const glm::vec3& d) { direction = d; }

    float getInnerConeAngle() const { return innerConeAngle; }
    void  setInnerConeAngle(float a) { innerConeAngle = a; }

    float getOuterConeAngle() const { return outerConeAngle; }
    void  setOuterConeAngle(float a) { outerConeAngle = a; }

    GPUSpotLight toGPUSpotLight(const glm::vec3& worldPosition) const;
    GPULight toGPULight(const glm::vec3& worldPosition) const override;

    // Use base class implementation for depth mapping resources
    void initDepthMappingResources(
        const sauce::LogicalDevice& logicalDevice,
        const sauce::PhysicalDevice& physicalDevice,
        const vk::raii::DescriptorPool& descriptorPool
    );

private:
    float range{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float innerConeAngle{0.0f};
    float outerConeAngle{0.7853981f}; // pi/4
};

}
