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
    // Note: using std430 we can tightly pack a float3 and a float together
    // and we use that here to avoid explicit padding between members.
    // That also means the order in which we define these attributes is important.
    struct GPULight {
        glm::vec3 position{0.0f};
        uint32_t type{0};
        glm::vec3 direction{0.0f, -1.0f, 0.0f};
        float intensity{1.0f};
        glm::vec3 color{1.0f};
        float range{0.0f};
        float innerConeAngle{0.0f};
        float outerConeAngle{0.0f};
        float _pad0[2]; // struct size must be a multiple of 16 bytes
    };
    static_assert(sizeof(GPULight) == 64, "GPULight must be 64 bytes to match shader_pbr.slang");

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
