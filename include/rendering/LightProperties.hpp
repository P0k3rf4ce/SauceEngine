#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

#include "modeling/ModelProperties.hpp"

namespace rendering
{

    // abstract base class for light properties
    class LightProperties
    {
    public:
        // construct with optional colour (defaults to white)
        explicit LightProperties(const glm::vec3 &colour = glm::vec3(1.0f));

        // construct with model properties
        LightProperties(const glm::vec3 &colour, const modeling::ModelProperties &modelProps);

        // virtual destructor for abstract base class
        virtual ~LightProperties() = 0;

        // accessors
        const glm::vec3 &getColour() const noexcept;
        void setColour(const glm::vec3 &colour) noexcept;

        // lifecycle methods that derived classes must implement
        virtual void load() = 0;
        virtual void unload() = 0;
        virtual void useBuffer() = 0;

        // shadow map getters
        GLuint getShadowDepthTexture() const { return depthMapTex; }
        GLuint getShadowFramebuffer() const { return depthMapFBO; }

    protected:
        glm::vec3 m_colour;

        GLuint depthMapFBO = 0;
        GLuint depthMapTex = 0;
        int shadowWidth = 1024;
        int shadowHeight = 1024;

        void initShadowResources();
        void destroyShadowResources();
    };

} // namespace rendering
