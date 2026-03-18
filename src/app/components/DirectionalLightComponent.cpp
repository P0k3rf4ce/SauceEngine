#include "app/components/DirectionalLightComponent.hpp"

namespace sauce {

    DirectionalLightComponent::DirectionalLightComponent()
        : LightComponent(LightType::Directional, "DirectionalLightComponent") {
    }

    DirectionalLightComponent::DirectionalLightComponent(const glm::vec3& color, float intensity)
        : LightComponent(LightType::Directional, "DirectionalLightComponent") {
        this->color = color;
        this->intensity = intensity;
    }

    GPULight DirectionalLightComponent::toGPULight(const glm::vec3& worldPosition) const {
        GPULight gpu{};
        gpu.position = worldPosition;
        gpu.type = static_cast<uint32_t>(LightType::Directional);
        gpu.direction =
            glm::vec3(0.0f, 0.0f, -1.0f); // Default direction, will be overridden by Scene
        gpu.intensity = intensity;
        gpu.color = color;
        gpu.range = 0.0f;
        gpu.innerConeAngle = 0.0f;
        gpu.outerConeAngle = 0.0f;
        return gpu;
    }

} // namespace sauce
