#pragma once

#include "app/Component.hpp"
#include <cstdint>
#include <glm/glm.hpp>

namespace sauce {

    enum class LightType : uint32_t {
        Directional = 0,
        Point = 1,
        Spot = 2,
    };

    // Matches the Light struct in shader_pbr.slang using scalar (tightly-packed) layout.
    // Used to populate the StructuredBuffer<Light> SSBO.
    struct GPULight {
        glm::vec3 position{0.0f};               // offset  0 (12 bytes)
        uint32_t type{0};                       // offset 12
        glm::vec3 direction{0.0f, -1.0f, 0.0f}; // offset 16
        float intensity{1.0f};                  // offset 28
        glm::vec3 color{1.0f};                  // offset 32
        float range{0.0f};                      // offset 44
        float innerConeAngle{0.0f};             // offset 48
        float outerConeAngle{0.0f};             // offset 52
    };
    static_assert(sizeof(GPULight) == 56,
                  "GPULight must be 56 bytes to match shader_pbr.slang scalar layout");

    class LightComponent : public Component {
      public:
        explicit LightComponent(LightType type, const std::string& name = "LightComponent")
            : Component(name), lightType(type) {
        }

        virtual ~LightComponent() = default;

        LightType getLightType() const {
            return lightType;
        }

        const glm::vec3& getColor() const {
            return color;
        }
        void setColor(const glm::vec3& c) {
            color = c;
        }

        float getIntensity() const {
            return intensity;
        }
        void setIntensity(float i) {
            intensity = i;
        }

        virtual GPULight toGPULight(const glm::vec3& worldPosition) const = 0;

      protected:
        LightType lightType;
        glm::vec3 color{1.0f};
        float intensity{1.0f};
    };

} // namespace sauce
