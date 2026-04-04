#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace sauce {
class RigidBodyComponent;
}

namespace physics {

struct ClothData;
struct Constraint;
struct Vertex;

struct XPBDSolver {
  // Gauss-Seidel iterations per rigid-body solve pass
  int solverIterations = 20;
  int rigidSubsteps = 4;

  // XPBD substeps for cloth (prediction + constraint projection each substep)
  int clothSubsteps = 4;

  // Normalized gravity direction for support-loss detection (default: -Z)
  glm::vec3 gravityDirection = glm::vec3(0.0f, 0.0f, -1.0f);

  // Dragging keeps the selected body dynamic, but its contacts can use a reduced friction
  // coefficient so blocks can be extracted without turning the full tower into a low-friction pile.
  const sauce::RigidBodyComponent* dragBody = nullptr;
  float dragContactFrictionScale = 0.02f;

  bool contactDebugEnabled = false;

  void solvePositions(std::vector<sauce::RigidBodyComponent*>& rigidBodies,
                      std::vector<std::unique_ptr<Constraint>>& constraints,
                      float deltatime);

  // Cloth-only pipeline: external acceleration, substepped XPBD on particle arrays (rigid bodies
  // untouched). Lambdas reset at the start of each substep.
  void solveCloth(ClothData& cloth, float deltatime,
                  const glm::vec3& externalAcceleration = glm::vec3(0.0f, -9.81f, 0.0f));

  void projectConstraints(
      std::vector<Vertex>& vertices,
      std::vector<std::unique_ptr<Constraint>>& constraints,
      float deltatime);

  std::vector<std::unique_ptr<Constraint>> generateCollisionConstraints(
      const std::vector<sauce::RigidBodyComponent*>& rigidBodies);

  void wakeUnsupportedBodies(
      const std::vector<sauce::RigidBodyComponent*>& rigidBodies) const;

private:
  void captureCollisionLambdaWarmStart(
      const std::vector<sauce::RigidBodyComponent*>& rigidBodies,
      const std::vector<std::unique_ptr<Constraint>>& constraints);

  struct CollisionContact {
    uint32_t indexA = 0;
    uint32_t indexB = 0;
    glm::vec3 pointA = glm::vec3(0.0f);
    glm::vec3 pointB = glm::vec3(0.0f);
    glm::vec3 contactNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    float penetrationDepth = 0.0f;
  };

  std::vector<CollisionContact> collectCollisionContacts(
      const std::vector<sauce::RigidBodyComponent*>& rigidBodies) const;
  void applyCollisionVelocityResponse(
      const std::vector<sauce::RigidBodyComponent*>& rigidBodies,
      const std::vector<CollisionContact>& contacts,
      float deltatime) const;

  std::map<std::pair<uintptr_t, uintptr_t>, std::vector<float>> collisionLambdaWarmStart;
};

} // namespace physics
