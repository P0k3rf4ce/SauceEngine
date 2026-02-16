#include <app/components/RigidBodyComponent.hpp>

#include <physics/constraints/Constraint.hpp>

namespace physics {

struct XPBDSolver {

  void solvePositions(std::vector<sauce::RigidBodyComponent>& rigidBodies, std::vector<Constraint>& constraints, float deltatime);

private:
  void projectConstraints(
      std::vector<Vertex>& vertices,
      std::vector<glm::vec3>& lagrangeMultipliers,
      std::vector<Constraint>& constraints,
      float deltatime
  );

  std::vector<Constraint> generateCollisionConstraints(std::vector<sauce::RigidBodyComponent>& rigidBodies);
};

}
