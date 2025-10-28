#include "LightProperties.hpp"
#include "utils/Logger.hpp"

namespace rendering
{

    LightProperties::LightProperties(const glm::vec3 &colour)
        : m_colour(colour) {}

    LightProperties::LightProperties(const glm::vec3 &colour, const modeling::ModelProperties &modelProps)
        : m_colour(colour)
    {
        initShadowResources();
    }

    LightProperties::~LightProperties()
    {
        destroyShadowResources();
    }

    const glm::vec3 &LightProperties::getColour() const noexcept
    {
        return m_colour;
    }

    void LightProperties::setColour(const glm::vec3 &colour) noexcept
    {
        m_colour = colour;
    }

    void LightProperties::initShadowResources()
    {
        if (depthMapFBO != 0 || depthMapTex != 0)
            return;

        glGenFramebuffers(1, &depthMapFBO);
        glGenTextures(1, &depthMapTex);

        glBindTexture(GL_TEXTURE_2D, depthMapTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0, 1.0, 1.0, 1.0};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // attach depth texture as FBO's depth buffer
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            LOG_ERROR("Shadow FBO incomplete: " + status);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteTextures(1, &depthMapTex);
            depthMapTex = 0;
            glDeleteFramebuffers(1, &depthMapFBO);
            depthMapFBO = 0;
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        LOG_INFO("Shadow map initialized.");
    }

    void LightProperties::destroyShadowResources()
    {
        if (depthMapTex != 0)
        {
            glDeleteTextures(1, &depthMapTex);
            depthMapTex = 0;
        }
        if (depthMapFBO != 0)
        {
            glDeleteFramebuffers(1, &depthMapFBO);
            depthMapFBO = 0;
        }
    }

} // namespace rendering
