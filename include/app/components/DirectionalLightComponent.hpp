#pragma once

#include "app/components/LightComponent.hpp"
#include <glm/glm.hpp>

namespace sauce {

    class DirectionalLightComponent : public LightComponent {
      public:
        DirectionalLightComponent();
        DirectionalLightComponent(const glm::vec3& color, float intensity);
        ~DirectionalLightComponent() override = default;

        const glm::vec3& getAmbient() const {
            return ambient;
        }
        const glm::vec3& getDiffuse() const {
            return diffuse;
        }
        const glm::vec3& getSpecular() const {
            return specular;
        }

        void setAmbient(const glm::vec3& a) {
            ambient = a;
        }
        void setDiffuse(const glm::vec3& d) {
            diffuse = d;
        }
        void setSpecular(const glm::vec3& s) {
            specular = s;
        }

        GPULight toGPULight(const glm::vec3& worldPosition) const override;

      private:
        glm::vec3 ambient{0.1f};
        glm::vec3 diffuse{1.0f};
        glm::vec3 specular{1.0f};
    };

} // namespace sauce
