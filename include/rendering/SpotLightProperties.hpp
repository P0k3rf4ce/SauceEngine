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

        void update(Scene& scene, animation::AnimationProperties& animProps) override;
        void confShadowMap(Scene& scene, Shader& shader) override;

        const glm::mat4& getLightSpaceMatrix() const;
        const glm::vec3& getPosition() const noexcept;
        const glm::vec3& getDirection() const noexcept;
        float getCutOff() const noexcept;
        float getOuterCutOff() const noexcept;

    private:
        void loadLightSpaceMatrix(const animation::AnimationProperties &animProps) override;

        glm::vec3 m_position;
        glm::vec3 m_direction;
        float m_cutOff;
        float m_outerCutOff;
        glm::mat4 m_lightSpaceMatrix;
    };
}
