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

  void solvePositions(std::vector<sauce::RigidBodyComponent>& rigidBodies, std::vector<Constraint>& constraints, float deltatime) {
	  /*
	   * adapted from https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
	   */
	  glm::vec3 velocity;
	  float w;

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

		  for (int i=0; i<solverIterations; i++) {
			  /*
			   * TODO
			   * generate center of mass from BVH spheres
			   * also label all the physics::Vertex instances explicitly wherever they appear
			   */
			  //projectConstraints(v, constraints, deltatime);
		  }
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
