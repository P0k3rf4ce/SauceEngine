#include <physics/constraints/Constraint.hpp>

#include <glm/glm.hpp>

namespace physics {

struct StretchConstraint : public Constraint {

  virtual void solve(std::vector<Vertex>& vertices, std::vector<glm::vec3>& lagrangeMultipliers, float deltatime) const override;

  float restLength;

};

}
