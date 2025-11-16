#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Eigen/Geometry>
#include "rendering/LightProperties.hpp"
#include "utils/Logger.hpp"
#include "utils/Shader.hpp"
#include "utils/EigenToGLM.hpp"

namespace rendering
{

    Shader *LightProperties::shader = nullptr;

    LightProperties::LightProperties(const glm::vec3 &colour)
        : m_colour(colour)
    {
        initShadowResources();
        this->initShader();
    }

    // provide definition for pure-virtual destructor
    LightProperties::~LightProperties() = default;

    const glm::vec3 &LightProperties::getColour() const noexcept
    {
        return m_colour;
    }

    void LightProperties::setColour(const glm::vec3 &colour) noexcept
    {
        m_colour = colour;
    }

    void LightProperties::initShader()
    {
        if (this->shader == nullptr)
        {
            std::unordered_map<SHADER_TYPE, std::string> shaderFiles = {
                {FRAGMENT, "src/rendering/shaders/shadow/shadow_mapping_depth.frag"},
                {VERTEX, "src/rendering/shaders/shadow/shadow_mapping_depth.vert"}};
            this->shader = new Shader();
            this->shader->loadFromFiles(shaderFiles);
        }
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

    void LightProperties::loadLightSpaceMatrix(const animation::AnimationProperties &animProps)
    {
        // get position from model matrix
        glm::vec3 position = E2GLM(animProps.getModelMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        // assume light faces towards origin
        glm::mat4 view = glm::lookAt(position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        Eigen::Matrix4f lightSpaceMatrix = this->projection * GLM2E(view);
        this->shader->setUniform("lightSpaceMatrix", lightSpaceMatrix);
    }

    // Get everything ready for rendering a shadow map
    void LightProperties::confShadowMap(const animation::AnimationProperties &animProps)
    {
        glViewport(0, 0, this->shadowWidth, this->shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, this->depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        this->shader->bind();
        this->loadLightSpaceMatrix(animProps);
    }

} // namespace rendering
