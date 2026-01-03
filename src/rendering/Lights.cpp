#include "rendering/Lights.hpp"
#include "utils/Logger.hpp"
#include "shared/Scene.hpp"
#include "utils/EigenToGLM.hpp"
#include "utils/Shader.hpp"
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

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
	// point light class
    PointLight::PointLight(const glm::vec3 &position, const glm::vec3 &colour)
        : LightProperties(colour), m_position(position)
    {
		// create and bind framebuffer, texture
        glGenFramebuffers(1, &m_depthMapFBO);
        glGenTextures(1, &m_depthMapTex);

        glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthMapTex);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		// attach cubemap to fbo
        glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthMapTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

		// make sure fbo is built
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            LOG_ERROR("Shadow cubemap FBO not complete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        LOG_INFO("Shadow cubemap initialized.");

		// compile shader
		this->initShader();
    }

    PointLight::~PointLight()
    {

    }

	void PointLight::initShader() {
	    if (!m_shader) {
			std::unordered_map<SHADER_TYPE, std::string> shaderFiles = {
				{FRAGMENT, "src/rendering/shaders/shadow/shadow_point.frag"},
				{GEOMETRY, "src/rendering/shaders/shadow/shadow_point.geom"},
				{VERTEX, "src/rendering/shaders/shadow/shadow_point.vert"}};
			m_shader = std::make_shared<Shader>();
			m_shader->loadFromFiles(shaderFiles);
	    }
	}

    void PointLight::update(std::shared_ptr<animation::AnimationProperties> &animProps)
    {
        m_position = glm::vec3(E2GLM(animProps->getModelMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		// extend(?) from abstract
		LightProperties::update(nullptr);
    }

    void PointLight::confShadowMap() {
		// extend from abstract
		LightProperties::confShadowMap();

		m_shader->bind();
		m_shader->setUniform("lightPos", m_position);
		m_shader->setUniform("farPlane", m_far);

		int i = 0;
		for (auto mat : m_lightSpaceMatrices)
			m_shader->setUniform("shadowMats[" + std::to_string(i) + "]", mat);
    }

    glm::vec3 &PointLight::getPosition() const noexcept {
        return m_position;
	}

	void PointLight::buildMatrix() {
		glm::mat4 proj = glm::perspective(glm::radians(90.0f), m_aspect, m_near, m_far);
		// one view matrix per face
		m_lightSpaceMatrices.push_back(proj *
				glm::lookAt(m_position, m_position + glm::vec3(1.0, 0.0, 0.0),
				glm::vec3(0.0, -1.0, 0.0)));
		m_lightSpaceMatrices.push_back(proj *
				glm::lookAt(m_position, m_position + glm::vec3(-1.0, 0.0, 0.0),
				glm::vec3(0.0, -1.0, 0.0)));
		m_lightSpaceMatrices.push_back(proj *
				glm::lookAt(m_position, m_position + glm::vec3(0.0, 1.0, 0.0),
				glm::vec3(0.0, -1.0, 0.0)));
		m_lightSpaceMatrices.push_back(proj *
				glm::lookAt(m_position, m_position + glm::vec3(0.0, -1.0, 0.0),
				glm::vec3(0.0, -1.0, 0.0)));
		m_lightSpaceMatrices.push_back(proj *
				glm::lookAt(m_position, m_position + glm::vec3(0.0, 0.0, 1.0),
				glm::vec3(0.0, -1.0, 0.0)));
		m_lightSpaceMatrices.push_back(proj *
				glm::lookAt(m_position, m_position + glm::vec3(0.0, 0.0, -1.0),
				glm::vec3(0.0, -1.0, 0.0)));
	}

	float PointLight::getNear() {
	    return m_near;
	}
	float PointLight::getFar() {
	    return m_far;
	}
	void PointLight::setNear(float f) {
	    m_near = f;
	}
	void PointLight::setFar(float f) {
	    m_far = f;
	}

	// dir light class
    DirLight::DirLight(const glm::vec3 &lightPos,
                 const glm::vec3 &colour)
        : LightProperties(colour), m_lightPos(lightPos)
    {

    }

    DirLight::~DirLight() {}

	void DirLight::initShader() {
	    if (!m_shader) {
			std::unordered_map<SHADER_TYPE, std::string> shaderFiles = {
				{FRAGMENT, "src/rendering/shaders/shadow/shadow_dir.frag"},
				{VERTEX, "src/rendering/shaders/shadow/shadow_dir.vert"}};
			m_shader = std::make_shared<Shader>();
			m_shader->loadFromFiles(shaderFiles);
	    }
	}

    void DirLight::confShadowMap() {
		// extend from abstract
		LightProperties::confShadowMap();

		// bind shader
		m_shader->bind();
		m_shader->setUniform("lightSpaceMatrix", m_lightSpaceMatrix);
    }

    void DirLight::setOrtho(float orthoSize, float nearPlane, float farPlane)
    {
        m_orthoSize = orthoSize;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
    }

	void DirLight::buildMatrix() {
		glm::mat4 proj = glm::ortho(
            -m_orthoSize, m_orthoSize,
            -m_orthoSize, m_orthoSize,
            m_nearPlane, m_farPlane);

		glm::mat4 view = glm::lookAt(
				m_lightPos,
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));

		m_lightSpaceMatrix = proj * view;
	}


	// spot light class
    SpotLight::SpotLight(const glm::vec3 &position, const glm::vec3 &direction, float cutOff, float outerCutOff, const glm::vec3 &colour)
        : LightProperties(colour), m_position(position), m_direction(glm::normalize(direction)), m_cutOff(cutOff), m_outerCutOff(outerCutOff)
    {}

    SpotLight::~SpotLight() {}

	void SpotLight::initShader() {
	    if (!m_shader) {
			std::unordered_map<SHADER_TYPE, std::string> shaderFiles = {
				{FRAGMENT, "src/rendering/shaders/shadow/shadow_dir.frag"},
				{VERTEX, "src/rendering/shaders/shadow/shadow_dir.vert"}};
			m_shader = std::make_shared<Shader>();
			m_shader->loadFromFiles(shaderFiles);
	    }
	}

    void SpotLight::update(animation::AnimationProperties &animProps)
    {
        m_position = E2GLM(animProps.getModelMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f).xyz;

		// extend(?) from abstract
		LightProperties::update(nullptr);
    }

    void SpotLight::confShadowMap() {
		// extend from abstract
		LightProperties::confShadowMap();

		// bind shader
		m_shader->bind();
		m_shader->setUniform("lightSpaceMatrix", m_lightSpaceMatrix);
    }

    const glm::mat4 &SpotLight::getLightSpaceMatrix() const
    {
        return m_lightSpaceMatrix;
    }

    const glm::vec3 &SpotLight::getPosition() const noexcept
    {
        return m_position;
    }

    const glm::vec3 &SpotLight::getDirection() const noexcept
    {
        return m_direction;
    }

    float SpotLight::getCutOff() const noexcept
    {
        return m_cutOff;
    }

    float SpotLight::getOuterCutOff() const noexcept
    {
        return m_outerCutOff;
    }

	void SpotLight::buildMatrix() {
		glm::mat4 proj = glm::perspective(
			2 * glm::radians(m_outerCutOff),
			m_aspect,
			m_nearPlane,
			m_farPlane);

		glm::mat4 view = glm::lookAt(
				m_position,
				m_position + m_direction,
				glm::vec3(0.0f, 1.0f, 0.0f));

		m_lightSpaceMatrix = proj * view;
	}
}
