#pragma once

#include <physics/ClothCollisionGeneration.hpp>

namespace physics {

// Generates normal-only static contact constraints for cloth particles.
std::vector<CollisionConstraint> generateClothCollisionConstraints(
    const ClothData& cloth,
    const std::vector<const Collider*>& staticColliders,
    float collisionThickness = 0.01f)
{
    std::vector<CollisionConstraint> constraints;

    for (uint32_t i = 0; i < cloth.particles.size(); ++i) {
        const ClothParticle& p = cloth.particles[i];

        // Skip particles that are pinned or have infinite mass
        if (p.isStatic()) {
            continue;
        }

        // Treat the cloth particle as a small sphere to generate contacts
        SphereCollider particleCollider;
        particleCollider.center = p.predictedPosition;
        particleCollider.radius = collisionThickness;

        for (const Collider* envCollider : staticColliders) {
            if (!envCollider) {
                continue;
            }

            std::vector<ContactInfo> contacts;
            
            // Check collision
            if (envCollider->checkCollision(particleCollider, contacts)) {
                for (const auto& contact : contacts) {
                    
                    // The target resting point is the particle's current position 
                    // pushed out along the normal by the exact penetration depth.
                    glm::vec3 resolutionPoint = p.predictedPosition + contact.contactNormal * contact.depth;
                    constraints.emplace_back(i, resolutionPoint, contact.contactNormal);
                }
            }
        }
    }

    return constraints;
}

} // namespace physics