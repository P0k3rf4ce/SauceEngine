#include "app/components/PointLightComponent.hpp"

namespace sauce {

    PointLightComponent::PointLightComponent()
        : LightComponent(LightType::Point, "PointLightComponent") {
    }

    PointLightComponent::PointLightComponent(const glm::vec3& color, float intensity, float range)
        : LightComponent(LightType::Point, "PointLightComponent"), range(range) {
        this->color = color;
        this->intensity = intensity;
    }

    GPUPointLight PointLightComponent::toGPUPointLight(const glm::vec3& worldPosition) const {
        GPUPointLight gpu{};
        gpu.position = worldPosition;
        gpu.color = color;
        gpu.ambient = ambient;
        gpu.diffuse = diffuse;
        gpu.specular = specular;
        gpu.constant = constant;
        gpu.linear = linear;
        gpu.quadratic = quadratic;
        return gpu;
    }

    GPULight PointLightComponent::toGPULight(const glm::vec3& worldPosition) const {
        GPULight gpu{};
        gpu.position = worldPosition;
        gpu.type = static_cast<uint32_t>(LightType::Point);
        gpu.direction = glm::vec3(0.0f);
        gpu.intensity = intensity;
        gpu.color = color;
        gpu.range = range;
        gpu.innerConeAngle = 0.0f;
        gpu.outerConeAngle = 0.0f;
        return gpu;
    }

} // namespace sauce
