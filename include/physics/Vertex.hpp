#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace physics {

struct Vertex {

  glm::vec3 position, velocity;
  float invMass;

  glm::quat orientation = glm::quat(glm::vec3(0.0f));
  glm::vec3 angularVelocity = glm::vec3(0.0f);
  glm::mat3 invInertiaTensor = glm::mat3(1.0f);

};

}
