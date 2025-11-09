#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <Eigen/Geometry>
#include "animation/AnimationProperties.hpp"
#include "utils/Shader.hpp"

namespace rendering
{

    // abstract base class for light properties
    class LightProperties
    {
    public:
        // construct with optional colour (defaults to white)
        explicit LightProperties(const glm::vec3 &colour = glm::vec3(1.0f));

        // virtual destructor for abstract base class
        virtual ~LightProperties() = 0;

        // accessors
        const glm::vec3 &getColour() const noexcept;
        void setColour(const glm::vec3 &colour) noexcept;

        // lifecycle update method that derived classes must implement
        virtual void update() = 0;

        // configure shadow map parameters (abstract)
        virtual void confShadowMap(const animation::AnimationProperties &animProps, Shader &shader);

    protected:
        glm::vec3 m_colour;

        // shadow map resources
        GLuint depthMapFBO = 0;
        GLuint depthMapTex = 0;
        const unsigned int shadowWidth = 1024;
        const unsigned int shadowHeight = 1024;

        // view transforms
        Eigen::Matrix4f projection;

    private:
        void initShadowResources();
    };

} // namespace rendering
