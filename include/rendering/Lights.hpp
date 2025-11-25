#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "animation/AnimationProperties.hpp"
#include "rendering/LightProperties.hpp"
#include "utils/Shader.hpp"
#include "shared/Scene.hpp"

namespace rendering
{

    class PointLight : public LightProperties
    {
    public:
        PointLight(const glm::vec3 &position, const glm::vec3 &colour = glm::vec3(1.0f));
        ~PointLight();

        void update(Scene &scene, animation::AnimationProperties &animProps) override;
        void confShadowMap(Scene &scene, Shader &shader) override;

        const glm::vec3 &getPosition() const noexcept;

    private:
        void initShadowCubemap();

        glm::vec3 m_position;
        GLuint m_cubemapFBO;
        GLuint m_cubemapTex;
        std::vector<glm::mat4> m_lightSpaceMatrices;
    };

    class DirLight : public LightProperties
    {
    public:
        DirLight(const glm::vec3 &lightPos,
                 const glm::vec3 &colour = glm::vec3(1.0f));
        ~DirLight() override;

        void update(Scene &scene, animation::AnimationProperties &animProps) override;
        void confShadowMap(Scene &scene, Shader &shader) override;

        const glm::vec3 &getLightPosition() const noexcept { return m_lightPos; }

        glm::vec3 getLightDirection() const noexcept { return glm::normalize(-m_lightPos); }

        void setLightPosition(const glm::vec3 &pos);
        void setOrtho(float orthoSize, float nearPlane, float farPlane);

    private:
        void rebuildProjectionIfNeeded();
        void rebuildProjection();

        glm::vec3 m_lightPos;
        float m_orthoSize = 10.0f;
        float m_nearPlane = 1.0f;
        float m_farPlane = 25.0f;

        // Rebuild flag
        mutable bool m_projectionNeedsRebuild = true;
    };
}
