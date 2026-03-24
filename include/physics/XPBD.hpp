#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
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
  int rigidSubsteps = 2;

  // XPBD substeps for cloth (prediction + constraint projection each substep)
  int clothSubsteps = 4;

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

private:
  void clearStaticContactState();
  void resetStaticContactState(size_t bodyCount);
  void recordStaticContact(uint32_t bodyIndex, const glm::vec3& normal);
  bool hasStaticContacts(size_t bodyIndex) const;
  const std::vector<glm::vec3>& staticContactNormals(size_t bodyIndex) const;
  void stabilizePostSolveVelocities(size_t bodyIndex,
                                    glm::vec3& linearVelocity,
                                    glm::vec3& angularVelocity) const;

  std::vector<std::vector<glm::vec3>> lastStaticContactNormals;
  std::vector<uint32_t> lastStaticContactCounts;
};

} // namespace physics
