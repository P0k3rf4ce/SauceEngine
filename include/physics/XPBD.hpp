#pragma once

#include <physics/Vertex.hpp>
#include <physics/constraints/Constraint.hpp>

#include <memory>
#include <vector>

namespace physics {

struct XPBDSolver {

  // Number of Gauss-Seidel iterations per substep (default 10)
  // we can probably hook up imgui to this at some point for tuning
  int solverIterations = 10;

  void projectConstraints(
      std::vector<Vertex>& vertices,
      std::vector<std::unique_ptr<Constraint>>& constraints,
      float deltatime
  ) {
    if (vertices.empty() || constraints.empty()) return;

    for (int iter = 0; iter < solverIterations; ++iter) {
      if (iter == 0) {
        for (auto& c : constraints) {
          c->resetLambda();
        }
      }

      for (auto& c : constraints) {
        c->solve(vertices, deltatime);
      }
    }
  }
};

}
