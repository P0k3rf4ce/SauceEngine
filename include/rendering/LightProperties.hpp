#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <Eigen/Geometry>
#include "shared/Scene.hpp"
#include "rendering/RenderProperties.hpp"
#include "animation/AnimationProperties.hpp"

namespace rendering
{

    // abstract base class for light properties
    class LightProperties
    {
    public:
        // construct with optional colour (defaults to white)
        explicit LightProperties(glm::vec3 &colour = glm::vec3(1.0f));

        // virtual destructor for abstract base class
        virtual ~LightProperties() = 0;

        // accessors
        glm::vec3 &getColour();
        void setColour(glm::vec3 &colour);
		glm::mat4 getLightSpaceMat();

		std::shared_ptr<RenderProperties> getLightObj();
		void setLightObj(std::shared_ptr<RenderProperties> obj);

        virtual void update(std::shared_ptr<AnimationProperties>& animProps = nullptr);

    protected:
        glm::vec3 m_colour;
		std::shared_ptr<RenderProperties> m_lightObject = nullptr;

        // shadow map resources
        GLuint m_depthMapFBO = 0;
        GLuint m_depthMapTex = 0;
        const unsigned int shadowWidth = 1024;
        const unsigned int shadowHeight = 1024;
		const float m_aspect = (float)shadowWidth/(float)shadowHeight;

		glm::mat4 m_lightSpaceMatrix;
        virtual void buildMatrix() = 0;
        virtual void initShader() = 0;
        virtual void confShadowMap();
    };

} // namespace rendering
