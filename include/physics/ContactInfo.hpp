

#include <glm/glm.hpp>

namespace physics {

struct SphereCollider;

struct ContactInfo {

  ContactInfo(
      glm::vec3 contactPoint,
      glm::vec3 contactNormal,
      std::shared_ptr<SphereCollider> sphere1,
      std::shared_ptr<SphereCollider> sphere2,
      float depth,
      float restitution = 1.0f,
      float friction = 0.0f
      ) : 
    contactPoint(contactPoint), contactNormal(contactNormal),
    sphere1(sphere1), sphere2(sphere2), depth(depth), restitution(restitution), friction(friction)
  {}

  glm::vec3 contactPoint;
  glm::vec3 contactNormal;

  std::shared_ptr<SphereCollider> sphere1, sphere2;

  float depth;
  float restitution;
  float friction;

};

}
