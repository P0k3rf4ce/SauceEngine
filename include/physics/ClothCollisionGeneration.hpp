#pragma once

#include <physics/Cloth.hpp>
#include <physics/Collider.hpp>
#include <physics/SphereCollider.hpp>
#include <physics/constraints/CollisionConstraint.hpp>
#include <physics/ContactInfo.hpp>

#include <vector>

namespace physics {

// Generates normal-only static contact constraints for cloth particles.
std::vector<CollisionConstraint> generateClothCollisionConstraints(
    const ClothData& cloth,
    const std::vector<const Collider*>& staticColliders,
    float collisionThickness = 0.01f
);
} // namespace physics