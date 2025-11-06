#include "rendering/Lights.hpp"
#include "utils/Logger.hpp"
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace rendering
{

    PointLight::PointLight(const glm::vec3 &position, const glm::vec3 &colour)
        : LightProperties(colour), m_position(position), m_cubemapFBO(0), m_cubemapTex(0)
    {
        initShadowCubemap();
    }

    PointLight::~PointLight()
    {
        glDeleteFramebuffers(1, &m_cubemapFBO);
        glDeleteTextures(1, &m_cubemapTex);
    }

    void PointLight::update()
    {
        // Not sure if this is needed... 
    }

    void PointLight::confShadowMap(Shader& shader)
    {
        float aspect = (float)shadowWidth / (float)shadowHeight;
        float near = 1.0f;
        float far = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);

        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(m_position, m_position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(m_position, m_position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(m_position, m_position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(m_position, m_position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(m_position, m_position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(m_position, m_position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

        for (unsigned int i = 0; i < 6; ++i)
            shader.setUniform("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);

        glViewport(0, 0, shadowWidth, shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, m_cubemapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    const glm::vec3 &PointLight::getPosition() const noexcept
    {
        return m_position;
    }

    void PointLight::initShadowCubemap()
    {
        if (m_cubemapFBO != 0 || m_cubemapTex != 0)
            return;

        glGenFramebuffers(1, &m_cubemapFBO);
        glGenTextures(1, &m_cubemapTex);

        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTex);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, m_cubemapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_cubemapTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            LOG_ERROR("Shadow cubemap FBO not complete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        LOG_INFO("Shadow cubemap initialized.");
    }

} // namespace rendering
