#pragma once

#include "app/Component.hpp"
#include <glm/glm.hpp>
#include <cstdint>

namespace sauce {

enum class LightType : uint32_t {
    Directional = 0,
    Point       = 1,
    Spot        = 2,
};

// Matches the Light struct in shader_pbr.slang (std430 layout, 80 bytes).
// Used to populate the StructuredBuffer<Light> SSBO.
struct alignas(16) GPULight {
    glm::vec3 position{0.0f};          // offset  0
    float     _pad0{0};                // offset 12
    uint32_t  type{0};                 // offset 16
    float     _pad1[3]{};              // offset 20
    glm::vec3 direction{0.0f, -1.0f, 0.0f}; // offset 32
    float     intensity{1.0f};         // offset 44
    glm::vec3 color{1.0f};             // offset 48
    float     range{0.0f};             // offset 60
    float     innerConeAngle{0.0f};    // offset 64
    float     outerConeAngle{0.0f};    // offset 68
    float     _pad2[2]{};              // offset 72
};
static_assert(sizeof(GPULight) == 80, "GPULight must be 80 bytes to match shader_pbr.slang");

class LightComponent : public Component {
public:
    explicit LightComponent(LightType type, const std::string& name = "LightComponent")
        : Component(name), lightType(type) {}

    virtual ~LightComponent() = default;

    LightType getLightType() const { return lightType; }

    const glm::vec3& getColor() const { return color; }
    void setColor(const glm::vec3& c) { color = c; }

    float getIntensity() const { return intensity; }
    void setIntensity(float i) { intensity = i; }

    virtual GPULight toGPULight(const glm::vec3& worldPosition) const = 0;

protected:
    LightType lightType;
    glm::vec3 color{1.0f};
    float intensity{1.0f};
};

} // namespace sauce
