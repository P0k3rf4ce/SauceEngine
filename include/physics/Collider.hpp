#pragma once

#include <physics/ContactInfo.hpp>

#include <glm/glm.hpp>

namespace physics {

    struct Collider {
        // Stores contact info if sphere intersects collider. If no intersection, info is not modified
        virtual bool checkCollision(const Collider& collider,
                                    std::vector<ContactInfo>& info) const = 0;
    };

} // namespace physics
