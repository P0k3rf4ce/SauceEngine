#include <physics/Collider.hpp>

#include <glm/glm.hpp>

namespace physics {

    struct SphereCollider : public Collider {
        // Stores contact info if spheres intersect. If no intersection, info is not modified
        virtual bool checkCollision(const Collider& collider,
                                    std::vector<ContactInfo>& info) const override;

        float radius = 1.0f;
        glm::vec3 center = glm::vec3(0.0f);
    };

} // namespace physics
