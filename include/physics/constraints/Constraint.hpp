#pragma once

#include <physics/Vertex.hpp>

#include <vector>

namespace physics {

struct Constraint {
  virtual ~Constraint() = default;
  virtual void solve(std::vector<Vertex>& vertices, float deltatime) = 0;
  void resetLambda() { lambda = 0.0f; }
  float compliance = 0.0f;

protected:
  float lambda = 0.0f;
};

}
