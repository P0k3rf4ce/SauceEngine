#pragma once

#include "rendering/LightProperties.hpp"
#include "utils/Shader.hpp"

#include <Eigen/Dense>

namespace rendering
{
    class DirLight : public LightProperties
    {
    public:
        DirLight(const glm::vec3 &lightPos,
                 const glm::vec3 &colour = glm::vec3(1.0f));
        ~DirLight();

        void update() override;
        /**
         * @brief Configure shadow mapping for the directional light.
         * Call shader.use() before invoking this method.
         *
         * @param shader The shader to configure.
         */
        void confShadowMap(Shader &shader) override;

        const glm::vec3 &getLightPosition() const noexcept { return m_lightPos; }
        const glm::mat4 &getLightSpaceMatrix() const noexcept { return m_lightSpaceMatrix; }

        /**
         * @brief Set the light position vector
         *
         * @param pos New position vector
         */
        void setLightPosition(const glm::vec3 &pos);
        /**
         * @brief Set the orthographic projection parameters for the light space matrix.
         *
         * @param orthoSize Size of the orthographic projection frustum
         * @param nearPlane Near plane distance
         * @param farPlane Far plane distance
         */
        void setOrtho(float orthoSize, float nearPlane, float farPlane);

    private:
        /**
         * @brief Compute the light space matrix for the directional light.
         *
         */
        void computeLightSpaceMatrix();

        // Where the light is positioned
        glm::vec3 m_lightPos;
        // light space transformation matrix
        glm::mat4 m_lightSpaceMatrix{1.0f};

        // Orthographic projection parameters
        float m_orthoSize = 10.0f;
        float m_nearPlane = 1.0f;
        float m_farPlane = 25.0f;
    };

}