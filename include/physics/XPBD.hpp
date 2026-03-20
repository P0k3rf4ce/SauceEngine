#pragma once

#include <app/Entity.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>

#include <physics/Vertex.hpp>
#include <physics/constraints/Constraint.hpp>

#include <memory>
#include <vector>

namespace physics {

struct XPBDSolver {

  // Number of Gauss-Seidel iterations per substep (default 10)
  // we can probably hook up imgui to this at some point for tuning
  int solverIterations = 10;

	void solvePositions(std::vector<sauce::RigidBodyComponent>& rigidBodies,
						std::vector<std::unique_ptr<Constraint>>& constraints,
						float deltatime) {
	/*
	* adapted from https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
	*/
	glm::vec3 velocity;
	float w;
	std::vector<physics::Vertex> centers; // centers of mass for all rigid bodies

	centers.reserve(rigidBodies.size());
	for (auto& r : rigidBodies) {
		auto o = r.getOwner();
		auto m = o ? o->getComponent<sauce::MeshRendererComponent>() : nullptr;
		if (!m || !m->getMesh())
		continue;

		auto v = m->getMesh()->getVertices();

		w = r.getInvMass();
		velocity = r.getVelocity() + deltatime * (w * r.getExternalForces());
		r.setPosition(r.getPosition() + velocity * deltatime);

		centers.push_back({ r.getCenterOfMass(), glm::vec3(0.f, 0.f, 0.f), r.getInvMass() });
	}

	// Generate constraints once, outside the loop
	constraints = generateCollisionConstraints(rigidBodies);

	for (int i = 0; i < solverIterations; i++) {
		projectConstraints(centers, constraints, deltatime);
	}
	}


  void projectConstraints(
      std::vector<physics::Vertex>& vertices,
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

  std::vector<std::unique_ptr<Constraint>> generateCollisionConstraints(std::vector<sauce::RigidBodyComponent>& rigidBodies);
};

}
