#pragma once

#include <glm/glm.hpp>

namespace physics {

struct Collider;

struct ContactInfo {

  ContactInfo(
      glm::vec3 contactPoint,
      glm::vec3 contactNormal,
      std::shared_ptr<Collider> pCollider1,
      std::shared_ptr<Collider> pCollider2,
      float depth,
      float restitution = 1.0f,
      float friction = 0.0f
      ) : 
    contactPoint(contactPoint), contactNormal(contactNormal),
    pCollider1(pCollider1), pCollider2(pCollider2), depth(depth), restitution(restitution), friction(friction)
  {}

  glm::vec3 contactPoint;
  glm::vec3 contactNormal;

  std::shared_ptr<Collider> pCollider1, pCollider2;

  float depth;
  float restitution;
  float friction;

};

}
