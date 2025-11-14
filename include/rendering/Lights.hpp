#pragma once

#include "rendering/LightProperties.hpp"
#include "utils/Shader.hpp"

#include "shared/Scene.hpp"
#include "animation/AnimationProperties.hpp"

namespace rendering
{

    class PointLight : public LightProperties
    {
    public:
        PointLight(const glm::vec3 &position, const glm::vec3 &colour = glm::vec3(1.0f));
        ~PointLight();

        void update(Scene& scene, animation::AnimationProperties& animProps) override;
        void confShadowMap(Scene& scene, Shader& shader) override;

        const glm::vec3 &getPosition() const noexcept;

    private:
        void initShadowCubemap();

        glm::vec3 m_position;
        GLuint m_cubemapFBO;
        GLuint m_cubemapTex;
        std::vector<glm::mat4> m_lightSpaceMatrices;
    };

} // namespace rendering
