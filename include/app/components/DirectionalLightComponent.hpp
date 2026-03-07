#pragma once

#include <glm/glm.hpp>
#include <app/Component.hpp>
#include <app/Vertex.hpp>

namespace sauce {

class DirectionalLightComponent : public Component {
public:
    DirectionalLightComponent(
        const glm::vec3& color = glm::vec3(1.0f),
        const glm::vec3& ambient = glm::vec3(0.1f),
        const glm::vec3& diffuse = glm::vec3(1.0f),
        const glm::vec3& specular = glm::vec3(1.0f)
    )
    : Component("DirectionalLightComponent")
    , color(color)
    , ambient(ambient)
    , diffuse(diffuse)
    , specular(specular)
    {}

    glm::vec3 color;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

} // namespace sauce
