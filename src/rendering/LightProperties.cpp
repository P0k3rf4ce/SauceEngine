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
    LightProperties::LightProperties(const glm::vec3 &colour)
        : m_colour(colour)
    {
		// create and bind framebuffer, texture
        glGenFramebuffers(1, &depthMapFBO);
        glGenTextures(1, &depthMapTex);
        glBindTexture(GL_TEXTURE_2D, depthMapTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        // attach texture to fbo
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

		// make sure fbo is built
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

		// compile shader
        this->initShader();
    }

    // provide definition for pure-virtual destructor
    LightProperties::~LightProperties() = default;

    glm::vec3 &LightProperties::getColour()
        return m_colour;

    void LightProperties::setColour(const glm::vec3 &colour)
        m_colour = colour;

	std::shared_ptr<RenderProperties> getLightObj()
	    return m_lightObject;

	void setLightObj(std::shared_ptr<RenderProperties> obj)
	    m_lightObject = obj;

	glm::mat4 getLightSpaceMat()
	    return m_lightSpaceMatrix;

    // Get everything ready for rendering a shadow map
    void LightProperties::confShadowMap() {
		// bind framebuffer
        glViewport(0, 0, this->shadowWidth, this->shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, this->depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

		// bind shader - done in derived class
    }

	// update this light's shadow map
	void LightProperties::update(std::shared_ptr<AnimationProperties>&) {
	    this->buildMatrix();
		this->confShadowMap();
		Scene::get_active_scene()->renderObjects(true, std::make_shared<LightProperties>(this));
	}

} // namespace rendering
