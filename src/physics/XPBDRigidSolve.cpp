#include <physics/XPBD.hpp>
#include <physics/Vertex.hpp>
#include <physics/constraints/Constraint.hpp>

#include <app/Entity.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace physics {

namespace {

struct RigidSolveBuffers {
  std::vector<physics::Vertex> vertices;
  std::vector<sauce::RigidBodyComponent*> rigidBodies;
};

std::vector<sauce::RigidBodyComponent*> collectSimulatedRigidBodies(
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies) {
  std::vector<sauce::RigidBodyComponent*> simulatedBodies;
  simulatedBodies.reserve(rigidBodies.size());

  for (auto* rigidBody : rigidBodies) {
    if (!rigidBody) {
      continue;
    }

    auto* owner = rigidBody->getOwner();
    auto* meshRenderer = owner ? owner->getComponent<sauce::MeshRendererComponent>() : nullptr;
    if (!meshRenderer || !meshRenderer->getMesh()) {
      continue;
    }

    simulatedBodies.push_back(rigidBody);
  }

  return simulatedBodies;
}

void initializeRigidSolveBuffers(const std::vector<sauce::RigidBodyComponent*>& rigidBodies,
                                 float deltaTime,
                                 RigidSolveBuffers& buffers) {
  buffers.vertices.clear();
  buffers.rigidBodies.clear();

  buffers.vertices.reserve(rigidBodies.size());
  buffers.rigidBodies.reserve(rigidBodies.size());

  for (auto* rigidBody : rigidBodies) {
    rigidBody->update(deltaTime);
    buffers.rigidBodies.push_back(rigidBody);
    buffers.vertices.push_back({
      rigidBody->getWorldCenterOfMass(),
      glm::vec3(0.0f),
      rigidBody->getInvMass(),
      rigidBody->getOrientation(),
      rigidBody->getAngularVelocity(),
      rigidBody->getInvInertiaTensor(),
    });
  }
}

void applyProjectedRigidState(const std::vector<physics::Vertex>& vertices,
                              const std::vector<sauce::RigidBodyComponent*>& rigidBodies) {
  const size_t bodyCount = std::min(vertices.size(), rigidBodies.size());
  for (size_t i = 0; i < bodyCount; ++i) {
    auto* rigidBody = rigidBodies[i];
    if (!rigidBody) {
      continue;
    }

    rigidBody->setOrientation(vertices[i].orientation);
    rigidBody->setWorldCenterOfMass(vertices[i].position);
  }
}

glm::vec3 reconstructAngularVelocity(const glm::quat& previousOrientation,
                                     const glm::quat& currentOrientation,
                                     float deltaTime) {
  if (deltaTime <= 0.0f) {
    return glm::vec3(0.0f);
  }

  const glm::quat dq = currentOrientation * glm::inverse(previousOrientation);
  const glm::quat normalizedDelta = glm::normalize(dq);
  const glm::vec3 imaginary(normalizedDelta.x, normalizedDelta.y, normalizedDelta.z);
  const float sinHalfAngle = glm::length(imaginary);
  if (sinHalfAngle <= 1e-8f) {
    return glm::vec3(0.0f);
  }

  const float angle = 2.0f * std::atan2(sinHalfAngle, normalizedDelta.w);
  const glm::vec3 axis = imaginary / sinHalfAngle;
  return axis * (angle / deltaTime);
}

glm::mat3 worldInvInertiaTensor(const sauce::RigidBodyComponent& rigidBody) {
  const glm::mat3 rotation = glm::mat3_cast(rigidBody.getOrientation());
  return rotation * rigidBody.getInvInertiaTensor() * glm::transpose(rotation);
}

glm::vec3 velocityAtPoint(const sauce::RigidBodyComponent& rigidBody, const glm::vec3& offset) {
  return rigidBody.getVelocity() + glm::cross(rigidBody.getAngularVelocity(), offset);
}

float angularEffectiveMass(const sauce::RigidBodyComponent& rigidBody,
                           const glm::vec3& offset,
                           const glm::vec3& direction) {
  if (rigidBody.getInvMass() <= 1e-8f) {
    return 0.0f;
  }

  const glm::mat3 worldInvInertia = worldInvInertiaTensor(rigidBody);
  const glm::vec3 angular = glm::cross(offset, direction);
  return glm::dot(glm::cross(worldInvInertia * angular, offset), direction);
}

void applyImpulse(sauce::RigidBodyComponent& rigidBody,
                  const glm::vec3& impulse,
                  const glm::vec3& offset) {
  if (rigidBody.getInvMass() <= 1e-8f) {
    return;
  }

  rigidBody.setVelocity(rigidBody.getVelocity() + rigidBody.getInvMass() * impulse);
  const glm::mat3 worldInvInertia = worldInvInertiaTensor(rigidBody);
  rigidBody.setAngularVelocity(
      rigidBody.getAngularVelocity() + worldInvInertia * glm::cross(offset, impulse));
}

} // namespace

void XPBDSolver::applyCollisionVelocityResponse(
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies,
    const std::vector<CollisionContact>& contacts,
    float deltatime) const {
  constexpr float kVelocityEpsilon = 1e-6f;
  constexpr float kBounceThreshold = 0.08f;
  constexpr float kFrictionCoefficient = 0.5f;
  constexpr float kSeparatingThreshold = 0.55f;
  constexpr int kVelocityPasses = 3;
  constexpr float kRelaxation = 0.55f;
  constexpr float kGravity = 9.81f;
  constexpr float kPenetrationBiasScale = 0.18f;
  constexpr float kPenetrationBiasMax = 0.45f;
  constexpr float kPenetrationBiasMinDepth = 2e-3f;

  const float h = std::max(deltatime, 1e-5f);

  for (int pass = 0; pass < kVelocityPasses; ++pass) {
    for (const auto& contact : contacts) {
      if (contact.indexA >= rigidBodies.size() || contact.indexB >= rigidBodies.size()) {
        continue;
      }

      auto* bodyA = rigidBodies[contact.indexA];
      auto* bodyB = rigidBodies[contact.indexB];
      if (!bodyA || !bodyB) {
        continue;
      }

      const float invMassA = bodyA->getInvMass();
      const float invMassB = bodyB->getInvMass();
      if (invMassA + invMassB <= 1e-8f) {
        continue;
      }

      const float normalLenSq = glm::dot(contact.contactNormal, contact.contactNormal);
      if (normalLenSq <= kVelocityEpsilon) {
        continue;
      }

      const glm::vec3 normal = contact.contactNormal / std::sqrt(normalLenSq);
      const glm::vec3 centerA = bodyA->getWorldCenterOfMass();
      const glm::vec3 centerB = bodyB->getWorldCenterOfMass();
      const glm::vec3 offsetA = contact.pointA - centerA;
      const glm::vec3 offsetB = contact.pointB - centerB;

      glm::vec3 relativeVelocity =
          velocityAtPoint(*bodyA, offsetA) - velocityAtPoint(*bodyB, offsetB);
      const float normalSpeed = glm::dot(relativeVelocity, normal);

      const float overlap = std::max(0.0f, contact.penetrationDepth);
      const float penetrationBias =
          (overlap < kPenetrationBiasMinDepth)
              ? 0.0f
              : std::min(kPenetrationBiasMax, overlap * kPenetrationBiasScale / h);
      const float effectiveNormalSpeed = normalSpeed - penetrationBias;

      float normalImpulseMagnitude = 0.0f;
      if (effectiveNormalSpeed < -kBounceThreshold) {
        const float normalMass = invMassA + invMassB +
            angularEffectiveMass(*bodyA, offsetA, normal) +
            angularEffectiveMass(*bodyB, offsetB, normal);
        if (normalMass > kVelocityEpsilon) {
          normalImpulseMagnitude = -effectiveNormalSpeed / normalMass;
          const glm::vec3 normalImpulse = kRelaxation * normalImpulseMagnitude * normal;
          applyImpulse(*bodyA, normalImpulse, offsetA);
          applyImpulse(*bodyB, -normalImpulse, offsetB);
          relativeVelocity = velocityAtPoint(*bodyA, offsetA) - velocityAtPoint(*bodyB, offsetB);
        }
      }

      const float normalSpeedAfter = glm::dot(relativeVelocity, normal);
      if (normalSpeedAfter > kSeparatingThreshold) {
        continue;
      }

      const glm::vec3 tangentialVelocity =
          relativeVelocity - glm::dot(relativeVelocity, normal) * normal;
      const float tangentialSpeedSq = glm::dot(tangentialVelocity, tangentialVelocity);
      if (tangentialSpeedSq <= kVelocityEpsilon) {
        continue;
      }

      const glm::vec3 tangent = tangentialVelocity / std::sqrt(tangentialSpeedSq);
      const float tangentialMass = invMassA + invMassB +
          angularEffectiveMass(*bodyA, offsetA, tangent) +
          angularEffectiveMass(*bodyB, offsetB, tangent);
      if (tangentialMass <= kVelocityEpsilon) {
        continue;
      }

      float frictionBasis = std::max(normalImpulseMagnitude, -std::min(normalSpeed, 0.0f));
      if (frictionBasis < kVelocityEpsilon) {
        const float effectiveMass = 1.0f / (invMassA + invMassB);
        frictionBasis = effectiveMass * kGravity * h * 0.3f;
      }
      const float tangentialImpulseLimit = kFrictionCoefficient * frictionBasis;
      const float tangentialImpulseMagnitude = std::clamp(
          -glm::dot(relativeVelocity, tangent) / tangentialMass,
          -tangentialImpulseLimit,
          tangentialImpulseLimit);
      const glm::vec3 tangentialImpulse = kRelaxation * tangentialImpulseMagnitude * tangent;
      applyImpulse(*bodyA, tangentialImpulse, offsetA);
      applyImpulse(*bodyB, -tangentialImpulse, offsetB);
    }
  }
}

void XPBDSolver::solvePositions(
    std::vector<sauce::RigidBodyComponent*>& rigidBodies,
    std::vector<std::unique_ptr<Constraint>>& constraints,
    float deltatime) {
  if (deltatime <= 0.0f) {
    return;
  }

  auto simulatedBodies = collectSimulatedRigidBodies(rigidBodies);
  if (simulatedBodies.empty()) {
    constraints.clear();
    return;
  }

  const int substeps = std::max(1, rigidSubsteps);
  const float h = deltatime / static_cast<float>(substeps);
  RigidSolveBuffers buffers;

  std::vector<glm::vec3> initialCenters;
  std::vector<glm::quat> initialOrientations;
  initialCenters.reserve(simulatedBodies.size());
  initialOrientations.reserve(simulatedBodies.size());
  for (auto* rigidBody : simulatedBodies) {
    initialCenters.push_back(rigidBody->getWorldCenterOfMass());
    initialOrientations.push_back(rigidBody->getOrientation());
  }

  constexpr float kMaxLinearSpeed = 8.0f;
  constexpr float kMaxAngularSpeed = 12.0f;

  auto clampVelocity = [](glm::vec3 v, float maxSpeed) -> glm::vec3 {
    const float speedSq = glm::length2(v);
    if (speedSq > maxSpeed * maxSpeed) {
      return v * (maxSpeed / std::sqrt(speedSq));
    }
    return v;
  };

  std::vector<glm::vec3> substepCenters(simulatedBodies.size());
  std::vector<glm::quat> substepOrientations(simulatedBodies.size());

  for (int substep = 0; substep < substeps; ++substep) {
    for (size_t i = 0; i < simulatedBodies.size(); ++i) {
      substepCenters[i] = simulatedBodies[i]->getWorldCenterOfMass();
      substepOrientations[i] = simulatedBodies[i]->getOrientation();
    }

    initializeRigidSolveBuffers(simulatedBodies, h, buffers);
    constraints = generateCollisionConstraints(buffers.rigidBodies);

    for (size_t i = 0; i < buffers.rigidBodies.size(); ++i) {
      auto* rb = buffers.rigidBodies[i];
      if (rb && buffers.vertices[i].invMass != rb->getInvMass()) {
        buffers.vertices[i].invMass = rb->getInvMass();
        buffers.vertices[i].invInertiaTensor = rb->getInvInertiaTensor();
      }
    }

    projectConstraints(buffers.vertices, constraints, h);
    applyProjectedRigidState(buffers.vertices, buffers.rigidBodies);

    for (size_t i = 0; i < simulatedBodies.size(); ++i) {
      auto* rigidBody = simulatedBodies[i];
      if (!rigidBody || rigidBody->getInvMass() <= 1e-8f) {
        continue;
      }
      rigidBody->setVelocity(clampVelocity(
          (rigidBody->getWorldCenterOfMass() - substepCenters[i]) / h,
          kMaxLinearSpeed));
      rigidBody->setAngularVelocity(clampVelocity(
          reconstructAngularVelocity(substepOrientations[i], rigidBody->getOrientation(), h),
          kMaxAngularSpeed));
    }
  }

  for (size_t i = 0; i < simulatedBodies.size(); ++i) {
    auto* rigidBody = simulatedBodies[i];
    if (!rigidBody) {
      continue;
    }

    if (rigidBody->getInvMass() <= 1e-8f) {
      rigidBody->setVelocity(glm::vec3(0.0f));
      rigidBody->setAngularVelocity(glm::vec3(0.0f));
      continue;
    }

    rigidBody->setVelocity(clampVelocity(
        (rigidBody->getWorldCenterOfMass() - initialCenters[i]) / deltatime,
        kMaxLinearSpeed));
    rigidBody->setAngularVelocity(clampVelocity(
        reconstructAngularVelocity(initialOrientations[i], rigidBody->getOrientation(), deltatime),
        kMaxAngularSpeed));
  }

  const auto frameContacts = collectCollisionContacts(simulatedBodies);
  applyCollisionVelocityResponse(simulatedBodies, frameContacts, deltatime);

  constexpr float kSleepLinearThreshold = 0.05f;
  constexpr float kSleepAngularThreshold = 0.1f;
  constexpr float kAutoSleepLinear = 0.012f;
  constexpr float kAutoSleepAngular = 0.024f;
  for (auto* rigidBody : simulatedBodies) {
    if (!rigidBody || rigidBody->getInvMass() <= 1e-8f) {
      continue;
    }
    const float linSpeedSq = glm::length2(rigidBody->getVelocity());
    const float angSpeedSq = glm::length2(rigidBody->getAngularVelocity());
    if (linSpeedSq < kSleepLinearThreshold * kSleepLinearThreshold) {
      rigidBody->setVelocity(rigidBody->getVelocity() * 0.86f);
    }
    if (angSpeedSq < kSleepAngularThreshold * kSleepAngularThreshold) {
      rigidBody->setAngularVelocity(rigidBody->getAngularVelocity() * 0.86f);
    }
    if (linSpeedSq < kAutoSleepLinear * kAutoSleepLinear &&
        angSpeedSq < kAutoSleepAngular * kAutoSleepAngular &&
        rigidBody->canBeDynamic()) {
      rigidBody->sleep();
    }
  }

  wakeUnsupportedBodies(simulatedBodies);
}

} // namespace physics
