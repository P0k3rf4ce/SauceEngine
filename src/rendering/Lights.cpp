#include "rendering/Lights.hpp"
#include "utils/Logger.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <Eigen/Geometry>
#include <glad/glad.h>

namespace
{
    /**
     * @brief Convert GLM mat4 to Eigen::Matrix4f.
     *
     * @param m GLM mat4 to convert.
     * @return Eigen::Matrix4f Converted Eigen matrix.
     */
    Eigen::Matrix4f toEigen(const glm::mat4 &m)
    {
        Eigen::Matrix4f e;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                e(r, c) = m[c][r]; // glm is indexed [column][row]
        return e;
    }
}

namespace rendering
{

    DirLight::DirLight(const glm::vec3 &lightPos, const glm::vec3 &colour)
        : LightProperties(colour), m_lightPos(lightPos)
    {
        computeLightSpaceMatrix();
    }

    DirLight::~DirLight() = default;

    void DirLight::update() {}

    void DirLight::confShadowMap(Shader &shader)
    {
        // Since shader.setUniform expects Eigen matrices
        const Eigen::Matrix4f lightSpaceEigen = toEigen(m_lightSpaceMatrix);
        shader.setUniform("lightSpaceMatrix", lightSpaceEigen);

        // Prepare the shadow map framebuffer
        glViewport(0, 0, shadowWidth, shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void DirLight::setLightPosition(const glm::vec3 &pos)
    {
        m_lightPos = pos;
        computeLightSpaceMatrix();
    }

    void DirLight::setOrtho(float orthoSize, float nearPlane, float farPlane)
    {
        m_orthoSize = orthoSize;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        computeLightSpaceMatrix();
    }

    void DirLight::computeLightSpaceMatrix()
    {
        // See default values in header file
        const glm::mat4 lightProj = glm::ortho(
            -m_orthoSize, m_orthoSize,
            -m_orthoSize, m_orthoSize,
            m_nearPlane, m_farPlane);

        const glm::mat4 lightView = glm::lookAt(
            m_lightPos,
            glm::vec3(0.0f, 0.0f, 0.0f), // scene's center
            glm::vec3(0.0f, 1.0f, 0.0f)  // up
        );

        m_lightSpaceMatrix = lightProj * lightView;
    }
}