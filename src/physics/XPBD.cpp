#include <physics/XPBD.hpp>
#include <physics/SphereCollider.hpp>
#include <physics/ContactInfo.hpp>
#include <physics/constraints/CollisionConstraint.hpp>

#include <app/Entity.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace physics {

std::vector<std::unique_ptr<Constraint>> XPBDSolver::generateCollisionConstraints(
    std::vector<sauce::RigidBodyComponent>& rigidBodies
) {
    std::vector<std::unique_ptr<Constraint>> constraints;

    // Build a bounding sphere for each rigid body from its mesh vertices.
    struct BodySphere {
        uint32_t index;
        SphereCollider sphere;
    };

    std::vector<BodySphere> bodies;
    bodies.reserve(rigidBodies.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(rigidBodies.size()); ++i) {
        auto& rb = rigidBodies[i];

        auto* owner = rb.getOwner();
        if (!owner) continue;

        auto* meshComp = owner->getComponent<sauce::MeshRendererComponent>();
        if (!meshComp || !meshComp->getMesh()) continue;

        const auto& verts = meshComp->getMesh()->getVertices();
        if (verts.empty()) continue;

        glm::vec3 center = rb.getPosition();

        float maxRadiusSq = 0.0f;
        for (const auto& v : verts) {
            float dSq = glm::length2(v.position - center);
            if (dSq > maxRadiusSq) {
                maxRadiusSq = dSq;
            }
        }

        SphereCollider sphere;
        sphere.center = center;
        sphere.radius = std::sqrt(maxRadiusSq);

        bodies.push_back({ i, sphere });
    }

    // Pairwise collision detection using SphereCollider::checkCollision.
    // For every contact produced, emit a CollisionConstraint.
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            std::vector<ContactInfo> contacts;

            if (!bodies[i].sphere.checkCollision(bodies[j].sphere, contacts)) {
                continue;
            }

            for (const auto& c : contacts) {
                constraints.push_back(std::make_unique<CollisionConstraint>(
                    bodies[i].index,
                    bodies[j].index,
                    c.contactNormal,
                    c.depth,
                    0.0f // zero compliance = perfectly rigid contact
                ));
            }
        }
    }

    return constraints;
}

} // namespace physics