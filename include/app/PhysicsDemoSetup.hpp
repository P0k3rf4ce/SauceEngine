#pragma once

#include <cstddef>
#include <vector>

namespace sauce {

class Entity;
class Scene;
class RigidBodyComponent;
class MeshRendererComponent;

namespace physics_demo {

struct SolverTuning {
  int solverIterations = 40;
  int rigidSubsteps = 8;
  float physicsDt = 1.0f / 128.0f;
};

SolverTuning selectRigidSolverTuning(size_t dynamicBodyCount);
bool isStaticTableMesh(const MeshRendererComponent* meshRenderer);
RigidBodyComponent* ensureEntityRigidBody(Entity& entity);
void configureRigidBodyFromEntity(Entity& entity, RigidBodyComponent& rigidBody);
void armScene(Scene& scene);
void applyForces(Scene& scene);
void syncRigidBodiesToTransforms(Scene& scene);
std::vector<RigidBodyComponent*> collectRigidBodies(Scene& scene);

} // namespace physics_demo

} // namespace sauce
