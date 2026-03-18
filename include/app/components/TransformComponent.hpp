#pragma once

#include "app/Component.hpp"
#include "app/modeling/Transform.hpp"

namespace sauce {

    class TransformComponent : public Component {
      public:
        TransformComponent();
        explicit TransformComponent(const modeling::Transform& transform);

        // Get the underlying transform
        modeling::Transform& getTransform() {
            return transform;
        }
        const modeling::Transform& getTransform() const {
            return transform;
        }

        // Convenience accessors
        glm::vec3 getTranslation() const {
            return transform.getTranslation();
        }
        glm::quat getRotation() const {
            return transform.getRotation();
        }
        glm::vec3 getScale() const {
            return transform.getScale();
        }

        void setTranslation(const glm::vec3& translation) {
            transform.setTranslation(translation);
        }
        void setRotation(const glm::quat& rotation) {
            transform.setRotation(rotation);
        }
        void setScale(const glm::vec3& scale) {
            transform.setScale(scale);
        }

        // Get the local transformation matrix
        glm::mat4 getLocalMatrix() const {
            return transform.getLocalMatrix();
        }

        // Component interface
        virtual ~TransformComponent() = default;

      private:
        modeling::Transform transform;
    };

} // namespace sauce
