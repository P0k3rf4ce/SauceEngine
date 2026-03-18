#include "app/components/TransformComponent.hpp"

namespace sauce {

    TransformComponent::TransformComponent() : transform() {
    }

    TransformComponent::TransformComponent(const modeling::Transform& transform)
        : transform(transform) {
    }

} // namespace sauce
