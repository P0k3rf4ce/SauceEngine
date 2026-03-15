#include "app/components/SpotLightComponent.hpp"

namespace sauce {

SpotLightComponent::SpotLightComponent()
    : LightComponent(LightType::Spot, "SpotLightComponent") {}

SpotLightComponent::SpotLightComponent(const glm::vec3& color, float intensity, float range,
                                       const glm::vec3& direction, float innerConeAngle,
                                       float outerConeAngle)
    : LightComponent(LightType::Spot, "SpotLightComponent")
    , range(range)
    , direction(direction)
    , innerConeAngle(innerConeAngle)
    , outerConeAngle(outerConeAngle)
{
    this->color = color;
    this->intensity = intensity;
}

GPUSpotLight SpotLightComponent::toGPUSpotLight(const glm::vec3& worldPosition) const {
    GPUSpotLight gpu{};
    gpu.position       = worldPosition;
    gpu.intensity      = intensity;
    gpu.direction      = direction;
    gpu.innerConeAngle = innerConeAngle;
    gpu.color          = color;
    gpu.outerConeAngle = outerConeAngle;
    gpu.range          = range;
    return gpu;
}

GPULight SpotLightComponent::toGPULight(const glm::vec3& worldPosition) const {
    GPULight gpu{};
    gpu.position       = worldPosition;
    gpu.type           = static_cast<uint32_t>(LightType::Spot);
    gpu.direction      = direction;
    gpu.intensity      = intensity;
    gpu.color          = color;
    gpu.range          = range;
    gpu.innerConeAngle = innerConeAngle;
    gpu.outerConeAngle = outerConeAngle;
    return gpu;
}

} // namespace sauce
