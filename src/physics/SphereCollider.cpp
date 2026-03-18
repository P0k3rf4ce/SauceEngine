#include <physics/SphereCollider.hpp>

#include <glm/glm.hpp>

namespace physics {

    const float eps = 1e-8f;

    bool SphereCollider::checkCollision(const Collider& collider,
                                        std::vector<ContactInfo>& info) const {
        const auto* other = dynamic_cast<const SphereCollider*>(&collider);
        if (!other) {
            // Delegate to the other collider's checkCollision (double dispatch)
            return collider.checkCollision(*this, info);
        }

        glm::vec3 delta = other->center - center;
        float distSq = glm::dot(delta, delta);
        float radiusSum = radius + other->radius;

        if (distSq > (radiusSum * radiusSum) + eps) {
            return false;
        }

        float dist = glm::sqrt(distSq);

        glm::vec3 normal;
        glm::vec3 contactPoint;
        float penetrationDepth = radiusSum - dist;

        if (dist < eps) {
            // Spheres are nearly coincident — pick an arbitrary normal
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
            contactPoint = center;
        } else {
            normal = delta / dist; // Points from this toward other
            contactPoint = center + normal * (radius - penetrationDepth * 0.5f);
        }

        info.emplace_back(contactPoint, normal, this, other, penetrationDepth);
        return true;
    }

} // namespace physics