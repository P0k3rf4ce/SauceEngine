#pragma once

#include <physics/constraints/Constraint.hpp>

#include <glm/glm.hpp>

namespace physics {

struct BendConstraint : public Constraint {

  void solve(std::vector<physics::Vertex>& vertices, float deltatime) override;

  float restAngle = 0.0f;

};

}
