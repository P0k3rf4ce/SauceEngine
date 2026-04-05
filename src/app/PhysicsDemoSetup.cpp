#include <app/PhysicsDemoSetup.hpp>

#include <app/Entity.hpp>
#include <app/Scene.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>
#include <app/components/TransformComponent.hpp>

#include <glm/glm.hpp>

#include <limits>

namespace sauce::physics_demo {

namespace {
bool isStaticTableMesh(const MeshRendererComponent* meshRenderer) {
  return meshRenderer && meshRenderer->getMesh() && meshRenderer->getMesh()->getVertexCount() == 4;
}
} // namespace

SolverTuning selectRigidSolverTuning(size_t dynamicBodyCount) {
  if (dynamicBodyCount >= 20) {
    return SolverTuning{
        .solverIterations = 30,
        .rigidSubsteps = 10,
        .physicsDt = 1.0f / 60.0f,
    };
  }

  if (dynamicBodyCount >= 12) {
    return SolverTuning{
        .solverIterations = 26,
        .rigidSubsteps = 6,
        .physicsDt = 1.0f / 72.0f,
    };
  }

  if (dynamicBodyCount >= 6) {
    return SolverTuning{
        .solverIterations = 22,
        .rigidSubsteps = 5,
        .physicsDt = 1.0f / 96.0f,
    };
  }

  return SolverTuning{};
}

void configureRigidBodyFromEntity(Entity& entity, RigidBodyComponent& rigidBody) {
  auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
  auto* transform = entity.getComponent<TransformComponent>();
  if (!meshRenderer || !meshRenderer->getMesh() || !transform) {
    return;
  }

  rigidBody.setPosition(transform->getTranslation());
  rigidBody.setOrientation(transform->getRotation());
  rigidBody.setScale(transform->getScale());
  const glm::vec3 centerOfMass = RigidBodyComponent::meshCenterOfMass(meshRenderer->getMesh());
  rigidBody.setCenterOfMass(centerOfMass);

  float invMass = 0.0f;
  if (!isStaticTableMesh(meshRenderer)) {
    invMass = RigidBodyComponent::scaledMeshInvMass(meshRenderer->getMesh(), transform->getScale());
    if (invMass <= std::numeric_limits<float>::epsilon()) {
      invMass = 1.0f;
    }
  }
  rigidBody.setMassProperties(
      invMass,
      RigidBodyComponent::meshInvInertiaTensor(
          meshRenderer->getMesh(),
          centerOfMass,
          invMass,
          transform->getScale()));
}

RigidBodyComponent* ensureEntityRigidBody(Entity& entity) {
  auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
  auto* transform = entity.getComponent<TransformComponent>();
  if (!meshRenderer || !meshRenderer->getMesh() || !transform) {
    return nullptr;
  }

  auto* rigidBody = entity.getComponent<RigidBodyComponent>();
  if (!rigidBody) {
    entity.addComponent<RigidBodyComponent>(
        transform->getTranslation(),
        glm::vec3(0.0f),
        transform->getRotation(),
        glm::vec3(0.0f));
    rigidBody = entity.getComponent<RigidBodyComponent>();
  }

  if (!rigidBody) {
    return nullptr;
  }

  configureRigidBodyFromEntity(entity, *rigidBody);
  return rigidBody;
}

void armScene(Scene& scene) {
  for (auto& entity : scene.getEntitiesMut()) {
    if (!entity.getActive()) {
      continue;
    }

    auto* rigidBody = ensureEntityRigidBody(entity);
    auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
    if (!rigidBody || !meshRenderer || !meshRenderer->getMesh()) {
      continue;
    }

    rigidBody->setVelocity(glm::vec3(0.0f));
    rigidBody->setAngularVelocity(glm::vec3(0.0f));
    rigidBody->clearAccumulatedForce();
    rigidBody->setCollisionEnabled(true);

    if (isStaticTableMesh(meshRenderer)) {
      rigidBody->setMassProperties(0.0f, glm::mat3(0.0f));
      continue;
    }

    rigidBody->sleep();
  }
}

void applyForces(Scene& scene) {
  constexpr glm::vec3 kGravityAcceleration(0.0f, 0.0f, -9.81f);
  constexpr float kLinearDamping = 0.8f;
  constexpr float kAngularDamping = 0.2f;

  for (auto& entity : scene.getEntitiesMut()) {
    auto* rigidBody = entity.getComponent<RigidBodyComponent>();
    if (!rigidBody || !entity.getActive()) {
      continue;
    }

    if (!rigidBody->isDynamic()) {
      rigidBody->clearAccumulatedForce();
      continue;
    }

    const glm::vec3 acceleration = kGravityAcceleration - rigidBody->getVelocity() * kLinearDamping;
    rigidBody->setAccumulatedForce(acceleration / rigidBody->getInvMass());

    rigidBody->setAngularVelocity(rigidBody->getAngularVelocity() * (1.0f - kAngularDamping));
  }
}

void syncRigidBodiesToTransforms(Scene& scene) {
  for (auto& entity : scene.getEntitiesMut()) {
    auto* rigidBody = entity.getComponent<RigidBodyComponent>();
    auto* transform = entity.getComponent<TransformComponent>();
    if (!rigidBody || !transform) {
      continue;
    }

    transform->setTranslation(rigidBody->getPosition());
    transform->setRotation(rigidBody->getOrientation());
  }
}

std::vector<RigidBodyComponent*> collectRigidBodies(Scene& scene) {
  std::vector<RigidBodyComponent*> rigidBodies;
  auto& entities = scene.getEntitiesMut();
  rigidBodies.reserve(entities.size());
  for (auto& entity : entities) {
    if (auto* rigidBody = entity.getComponent<RigidBodyComponent>()) {
      rigidBodies.push_back(rigidBody);
    }
  }
  return rigidBodies;
}

} // namespace sauce::physics_demo
