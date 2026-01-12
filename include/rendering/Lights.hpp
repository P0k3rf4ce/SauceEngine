#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "animation/AnimationProperties.hpp"
#include "rendering/LightProperties.hpp"
#include "utils/Shader.hpp"
#include "shared/Scene.hpp"

namespace rendering
{
	// point light class
    class PointLight : public LightProperties
    {
    public:
        PointLight(const glm::vec3 &position, const glm::vec3 &colour = glm::vec3(1.0f));
        ~PointLight();

        void update(const std::shared_ptr<AnimationProperties> &animProps) override;
        void confShadowMap() override;

		float getNear();
		float getFar();
		void setNear(float f);
		void setFar(float f);

        glm::vec3 &getPosition();

    private:
        glm::vec3 m_position;
        std::vector<glm::mat4> m_lightSpaceMatrices;

		float m_near = 1.0f;
		float m_far = 25.0f;

		void buildMatrix() override;
		void initShader();

		static std::shared_ptr<Shader> m_shader;
    };

	// dir light class
    class DirLight : public LightProperties
    {
    public:
        DirLight(const glm::vec3 &lightPos,
                 const glm::vec3 &colour = glm::vec3(1.0f));
        ~DirLight() override;

        void confShadowMap() override;

        glm::vec3 &getLightPosition() { return m_lightPos; }

        glm::vec3 getLightDirection() { return glm::normalize(-m_lightPos); }

        void setLightPosition(glm::vec3 &pos);
        void setOrtho(float orthoSize, float near, float far);

    private:
        glm::vec3 m_lightPos;
        float m_orthoSize = 10.0f;
        float m_near = 1.0f;
        float m_far = 25.0f;

		void buildMatrix() override;
		void initShader();

		static std::shared_ptr<Shader> m_shader;
    };


	// spot light class
    class SpotLight : public LightProperties
    {
    public:
        SpotLight(const glm::vec3& position, const glm::vec3& direction, float cutOff, float outerCutOff, const glm::vec3& colour = glm::vec3(1.0f));
        ~SpotLight() override;

        void update(const std::shared_ptr<AnimationProperties> &animProps) override;
        void confShadowMap() override;

        glm::mat4& getLightSpaceMatrix();
        glm::vec3& getPosition();
        glm::vec3& getDirection();
        float getCutOff();
        float getOuterCutOff();

    private:
        glm::vec3 m_position;
        glm::vec3 m_direction;
        float m_cutOff;
        float m_outerCutOff;
		float m_near = 1.0f;
		float m_far = 25.0f;

		void buildMatrix() override;
		void initShader();

		static std::shared_ptr<Shader> m_shader;
    };
}
