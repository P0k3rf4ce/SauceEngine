#include <physics/ContactInfo.hpp>

#include <glm/glm.hpp>

namespace physics {

struct SphereCollider {

  // Stores contact info if spheres intersect. If no intersection, info is not modified
  static bool checkIntersection(const SphereCollider& sc1, const SphereCollider& sc2, ContactInfo& info);

  float radius;
  glm::vec3 center;

};

}

