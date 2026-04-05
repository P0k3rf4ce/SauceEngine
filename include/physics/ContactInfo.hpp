#pragma once

#include <glm/glm.hpp>

namespace physics {

struct Collider;

struct ContactInfo {

    ContactInfo(
        const glm::vec3& contactPoint,
        const glm::vec3& contactNormal,
        const Collider* collider1,
        const Collider* collider2,
        float depth,
        float restitution = 1.0f,
        float friction = 0.0f
    ) :
        contactPoint(contactPoint),
        contactNormal(contactNormal),
        pCollider1(collider1),
        pCollider2(collider2),
        depth(depth),
        restitution(restitution),
        friction(friction)
    {}

    glm::vec3 contactPoint;
    glm::vec3 contactNormal;

    const Collider* pCollider1;
    const Collider* pCollider2;

    float depth;
    float restitution;
    float friction;
};

}
