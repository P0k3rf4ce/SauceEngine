#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace sauce {
namespace modeling {

// Forward declaration
class ModelNode;

class Transform {
public:
    Transform();
    Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);

    // Getters
    const glm::vec3& getTranslation() const { return translation; }
    const glm::quat& getRotation() const { return rotation; }
    const glm::vec3& getScale() const { return scale; }

    // Setters
    void setTranslation(const glm::vec3& translation);
    void setRotation(const glm::quat& rotation);
    void setScale(const glm::vec3& scale);

    // Matrix computation
    glm::mat4 getLocalMatrix() const;
    glm::mat4 getWorldMatrix(const ModelNode* node) const;

    // Static utility for matrix decomposition
    static Transform fromMatrix(const glm::mat4& matrix);

private:
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

} // namespace modeling
} // namespace sauce
