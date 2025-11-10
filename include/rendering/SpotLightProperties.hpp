#pragma once

#include "rendering/LightProperties.hpp"
#include <glm/glm.hpp>

namespace rendering
{
    class SpotLightProperties : public LightProperties
    {
    public:
        SpotLightProperties(const glm::vec3& position, const glm::vec3& direction, float cutOff, float outerCutOff, const glm::vec3& colour = glm::vec3(1.0f));
        ~SpotLightProperties() override;

        void update() override;
        void confShadowMap() override;

        const glm::mat4& getLightSpaceMatrix() const;

    private:
        glm::vec3 m_position;
        glm::vec3 m_direction;
        float m_cutOff;
        float m_outerCutOff;
        glm::mat4 m_lightSpaceMatrix;
    };
}
