#include "rendering/SpotLightProperties.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "utils/EigenToGLM.hpp"
#include "utils/Shader.hpp"

struct SpotLightGpuData
{
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 color;
    glm::mat4 lightSpaceMatrix;
    glm::vec4 cutoff;
};

namespace rendering
{
    SpotLightProperties::SpotLightProperties(const glm::vec3 &position, const glm::vec3 &direction, float cutOff, float outerCutOff, const glm::vec3 &colour)
        : LightProperties(colour), m_position(position), m_direction(glm::normalize(direction)), m_cutOff(cutOff), m_outerCutOff(outerCutOff)
    {}

    SpotLightProperties::~SpotLightProperties() {}

    void SpotLightProperties::update(Scene &scene, animation::AnimationProperties &animProps)
    {
        glm::mat4 model = E2GLM(animProps.getModelMatrix());
        glm::vec4 lightOrigin = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        m_position = glm::vec3(lightOrigin);

        const float nearPlane = 1.0f;
        const float farPlane = 25.0f;
        glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f), static_cast<float>(shadowWidth) / static_cast<float>(shadowHeight), nearPlane, farPlane);
        glm::mat4 lightView = glm::lookAt(m_position, m_position + m_direction, glm::vec3(0.0f, 1.0f, 0.0f));
        m_lightSpaceMatrix = lightProjection * lightView;

        LightProperties::confShadowMap(animProps);
        scene.draw(*shader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SpotLightProperties::confShadowMap(Scene &scene, Shader &shaderOverride)
    {
        shaderOverride.setUniform("lightSpaceMatrix", m_lightSpaceMatrix);
        scene.draw(shaderOverride);
    }

    const glm::mat4 &SpotLightProperties::getLightSpaceMatrix() const
    {
        return m_lightSpaceMatrix;
    }

    const glm::vec3 &SpotLightProperties::getPosition() const noexcept
    {
        return m_position;
    }

    const glm::vec3 &SpotLightProperties::getDirection() const noexcept
    {
        return m_direction;
    }

    float SpotLightProperties::getCutOff() const noexcept
    {
        return m_cutOff;
    }

    float SpotLightProperties::getOuterCutOff() const noexcept
    {
        return m_outerCutOff;
    }

    void SpotLightProperties::loadLightSpaceMatrix(const animation::AnimationProperties & /*animProps*/)
    {
        Eigen::Matrix4f lightSpaceEigen = GLM2E(m_lightSpaceMatrix);
        Eigen::Affine3d lightSpaceAffine = Eigen::Affine3d::Identity();
        lightSpaceAffine.matrix() = lightSpaceEigen.cast<double>();
        shader->setUniform("lightSpaceMatrix", lightSpaceAffine);
    }
}
