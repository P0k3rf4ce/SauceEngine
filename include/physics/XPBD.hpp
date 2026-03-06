#pragma once

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

  void solvePositions(std::vector<sauce::RigidBodyComponent>& rigidBodies, std::vector<std::unique_ptr<Constraint>>& constraints, float deltatime) {
	  /*
	   * adapted from https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
	   */
	  glm::vec3 velocity;
	  float w;
	  std::vector<physics::Vertex> centers; // centers of mass for all rigid bodies

	  centers.reserve(rigidBodies.size());
	  for (auto& r : rigidBodies) {
		  /* get vertices of r */
		  auto o = r.getOwner();
		  auto m = o->getComponent<sauce::MeshRendererComponent>();
		  /* skip rigid bodies with no mesh, we cannot generate their constraints */
		  if (m == nullptr)
			  continue;
		  auto v=m->getMesh()->getVertices();

		  /* do physics */
		  w=r.getInvMass();
		  velocity=r.getVelocity() + deltatime*(w*r.getExternalForces());
		  /*
		   * you could damp velocities here
		   */
		  r.setPosition(r.getPosition() + velocity*deltatime);
		  auto constraints=generateCollisionConstraints(rigidBodies);

		  centers.push_back({r.getCenterOfMass(), glm::vec3(0.f,0.f,0.f), r.getInvMass()});
	  }

	  for (int i=0; i<solverIterations; i++) {
		  projectConstraints(centers, constraints, deltatime);
	  }
  }

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

  std::vector<Constraint> generateCollisionConstraints(std::vector<sauce::RigidBodyComponent>& rigidBodies);
};

}
