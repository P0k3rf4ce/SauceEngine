#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace sauce {
struct ClothSettings;
class RigidBodyComponent;
}

namespace physics {

struct ClothData;
struct Constraint;
struct Vertex;

struct XPBDSolver {

  // Gauss-Seidel iterations per rigid-body solve pass
  int solverIterations = 10;

  void solvePositions(std::vector<sauce::RigidBodyComponent>& rigidBodies,
                      std::vector<std::unique_ptr<Constraint>>& constraints,
                      float deltatime);

  // Cloth-only pipeline: external acceleration, substepped XPBD on particle arrays (rigid bodies
  // untouched). Lambdas reset at the start of each substep.
  void solveCloth(ClothData& cloth,
                  const sauce::ClothSettings& settings,
                  float deltatime,
                  const glm::vec3& externalAcceleration = glm::vec3(0.0f, 0.0f, -9.81f));

  void projectConstraints(
      std::vector<Vertex>& vertices,
      std::vector<std::unique_ptr<Constraint>>& constraints,
      float deltatime);

  std::vector<std::unique_ptr<Constraint>> generateCollisionConstraints(
      std::vector<sauce::RigidBodyComponent>& rigidBodies);
};

} // namespace physics
