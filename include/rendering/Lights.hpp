#pragma once

#include "rendering/LightProperties.hpp"
#include "utils/Shader.hpp"

namespace rendering
{

    class PointLight : public LightProperties
    {
    public:
        PointLight(const glm::vec3 &position, const glm::vec3 &colour = glm::vec3(1.0f));
        ~PointLight() override;

        void update() override;
        void confShadowMap(Shader& shader) override;

        const glm::vec3 &getPosition() const noexcept;

    private:
        void initShadowCubemap();

        glm::vec3 m_position;
        GLuint m_cubemapFBO;
        GLuint m_cubemapTex;
    };

} // namespace rendering
