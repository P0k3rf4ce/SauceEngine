#pragma once

#include <glm/glm.hpp>

namespace rendering {

// abstract base class for light properties
class LightProperties {
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
    virtual void confShadowMap() = 0;

protected:
    glm::vec3 m_colour;
};

} // namespace rendering
