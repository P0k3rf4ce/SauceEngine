#include "rendering/SpotLightProperties.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace rendering
{
    SpotLightProperties::SpotLightProperties(const glm::vec3& position, const glm::vec3& direction, float cutOff, float outerCutOff, const glm::vec3& colour)
        : LightProperties(colour), m_position(position), m_direction(direction), m_cutOff(cutOff), m_outerCutOff(outerCutOff)
    {
    }

    SpotLightProperties::~SpotLightProperties()
    {
    }

    void SpotLightProperties::update()
    {
        float near_plane = 1.0f, far_plane = 7.5f;
        glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f), (float)shadowWidth / (float)shadowHeight, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(m_position, m_position + m_direction, glm::vec3(0.0, 1.0, 0.0));
        m_lightSpaceMatrix = lightProjection * lightView;
    }

    void SpotLightProperties::confShadowMap()
    {
        glViewport(0, 0, shadowWidth, shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    const glm::mat4& SpotLightProperties::getLightSpaceMatrix() const
    {
        return m_lightSpaceMatrix;
    }
}
