#include "LightProperties.hpp"

namespace rendering {

LightProperties::LightProperties(const glm::vec3 &colour)
    : m_colour(colour) {}

// provide definition for pure-virtual destructor
LightProperties::~LightProperties() = default;

const glm::vec3 &LightProperties::getColour() const noexcept {
    return m_colour;
}

void LightProperties::setColour(const glm::vec3 &colour) noexcept {
    m_colour = colour;
}

} // namespace rendering
