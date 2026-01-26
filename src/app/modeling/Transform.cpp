#include "app/modeling/Transform.hpp"
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace sauce {
namespace modeling {

// Forward declaration for ModelNode - will be fully defined in ModelNode.hpp
class ModelNode;

Transform::Transform()
    : translation(0.0f, 0.0f, 0.0f)
    , rotation(1.0f, 0.0f, 0.0f, 0.0f)  // Identity quaternion
    , scale(1.0f, 1.0f, 1.0f) {
}

Transform::Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
    : translation(translation)
    , rotation(rotation)
    , scale(scale) {
}

void Transform::setTranslation(const glm::vec3& translation) {
    this->translation = translation;
}

void Transform::setRotation(const glm::quat& rotation) {
    this->rotation = rotation;
}

void Transform::setScale(const glm::vec3& scale) {
    this->scale = scale;
}

glm::mat4 Transform::getLocalMatrix() const {
    glm::mat4 matrix(1.0f);
    matrix = glm::translate(matrix, translation);
    matrix = matrix * glm::mat4_cast(rotation);
    matrix = glm::scale(matrix, scale);
    return matrix;
}

glm::mat4 Transform::getWorldMatrix(const ModelNode* node) const {
    // This will be implemented once ModelNode is defined
    // For now, just return local matrix
    // The actual implementation will traverse up the parent chain
    return getLocalMatrix();
}

Transform Transform::fromMatrix(const glm::mat4& matrix) {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(matrix, scale, rotation, translation, skew, perspective);

    return Transform(translation, rotation, scale);
}

} // namespace modeling
} // namespace sauce
