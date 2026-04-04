#include <physics/XPBD.hpp>
#include <physics/Vertex.hpp>
#include <physics/constraints/CollisionConstraint.hpp>
#include <physics/constraints/Constraint.hpp>

#include <app/Entity.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

namespace physics {

namespace {

struct RigidSolveBuffers {
  std::vector<physics::Vertex> vertices;
  std::vector<sauce::RigidBodyComponent*> rigidBodies;
  std::vector<glm::vec3> previousCentersOfMass;
  std::vector<glm::quat> previousOrientations;
};

glm::vec3 clampMagnitude(const glm::vec3& value, float maxMagnitude) {
  const float lengthSq = glm::length2(value);
  if (maxMagnitude <= 0.0f || lengthSq <= maxMagnitude * maxMagnitude) {
    return value;
  }
  return value * (maxMagnitude / std::sqrt(lengthSq));
}

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
  buffers.previousCentersOfMass.clear();
  buffers.previousOrientations.clear();

  buffers.vertices.reserve(rigidBodies.size());
  buffers.rigidBodies.reserve(rigidBodies.size());
  buffers.previousCentersOfMass.reserve(rigidBodies.size());
  buffers.previousOrientations.reserve(rigidBodies.size());

  for (auto* rigidBody : rigidBodies) {
    buffers.previousCentersOfMass.push_back(rigidBody->getWorldCenterOfMass());
    buffers.previousOrientations.push_back(rigidBody->getOrientation());
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

glm::vec3 angularVelocityFromSolvedOrientation(const glm::quat& previousOrientation,
                                               const glm::quat& solvedOrientation,
                                               float deltaTime) {
  constexpr float kAngularEpsilon = 1e-6f;

  if (deltaTime <= 0.0f) {
    return glm::vec3(0.0f);
  }

  glm::quat delta = glm::normalize(solvedOrientation * glm::inverse(previousOrientation));
  if (delta.w < 0.0f) {
    delta = -delta;
  }

  const glm::vec3 deltaImag(delta.x, delta.y, delta.z);
  const float sinHalfAngle = glm::length(deltaImag);
  if (sinHalfAngle <= kAngularEpsilon) {
    return (2.0f / deltaTime) * deltaImag;
  }

  const float angle = 2.0f * std::atan2(sinHalfAngle, delta.w);
  return (angle / (deltaTime * sinHalfAngle)) * deltaImag;
}

void applyProjectedRigidState(const std::vector<physics::Vertex>& vertices,
                              const std::vector<sauce::RigidBodyComponent*>& rigidBodies,
                              const std::vector<glm::vec3>& previousCentersOfMass,
                              const std::vector<glm::quat>& previousOrientations,
                              float deltaTime) {
  constexpr float kMaxProjectedLinearSpeed = 18.0f;
  constexpr float kMaxProjectedAngularSpeed = 24.0f;
  const size_t bodyCount = std::min(vertices.size(), rigidBodies.size());
  for (size_t i = 0; i < bodyCount; ++i) {
    auto* rigidBody = rigidBodies[i];
    if (!rigidBody) {
      continue;
    }

    const glm::quat solvedOrientation = glm::normalize(vertices[i].orientation);
    const glm::vec3 solvedCenterOfMass = vertices[i].position;

    rigidBody->setOrientation(solvedOrientation);
    rigidBody->setWorldCenterOfMass(solvedCenterOfMass);

    if (rigidBody->getInvMass() <= 1e-8f || deltaTime <= 0.0f) {
      rigidBody->setVelocity(glm::vec3(0.0f));
      rigidBody->setAngularVelocity(glm::vec3(0.0f));
      continue;
    }

    if (i < previousCentersOfMass.size()) {
      rigidBody->setVelocity(clampMagnitude(
          (solvedCenterOfMass - previousCentersOfMass[i]) / deltaTime,
          kMaxProjectedLinearSpeed));
    }
    if (i < previousOrientations.size()) {
      rigidBody->setAngularVelocity(clampMagnitude(
          angularVelocityFromSolvedOrientation(previousOrientations[i], solvedOrientation, deltaTime),
          kMaxProjectedAngularSpeed));
    }
  }
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

std::pair<uintptr_t, uintptr_t> canonicalRigidPair(const sauce::RigidBodyComponent* a,
                                                    const sauce::RigidBodyComponent* b) {
  uintptr_t pa = reinterpret_cast<uintptr_t>(a);
  uintptr_t pb = reinterpret_cast<uintptr_t>(b);
  if (pa > pb) {
    std::swap(pa, pb);
  }
  return {pa, pb};
}

std::tuple<int, int, int> quantizedWorldMid(const glm::vec3& mid) {
  auto q = [](float x) { return static_cast<int>(std::lround(x * 1000.0f)); };
  return {q(mid.x), q(mid.y), q(mid.z)};
}

} // namespace

void XPBDSolver::captureCollisionLambdaWarmStart(
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies,
    const std::vector<std::unique_ptr<Constraint>>& constraints) {
  struct Row {
    std::pair<uintptr_t, uintptr_t> pairKey;
    std::tuple<int, int, int> midQ;
    float lambda = 0.0f;
  };
  std::vector<Row> rows;
  rows.reserve(constraints.size());

  for (const auto& up : constraints) {
    const auto* cc = dynamic_cast<const CollisionConstraint*>(up.get());
    if (!cc) {
      continue;
    }
    if (cc->indexA >= rigidBodies.size() || cc->indexB >= rigidBodies.size()) {
      continue;
    }
    auto* ba = rigidBodies[cc->indexA];
    auto* bb = rigidBodies[cc->indexB];
    if (!ba || !bb) {
      continue;
    }
    rows.push_back(Row{
        canonicalRigidPair(ba, bb),
        quantizedWorldMid(cc->warmSortMid),
        cc->accumulatedLambda(),
    });
  }

  std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
    if (a.pairKey != b.pairKey) {
      return a.pairKey < b.pairKey;
    }
    return a.midQ < b.midQ;
  });

  collisionLambdaWarmStart.clear();
  for (size_t i = 0; i < rows.size();) {
    const auto pk = rows[i].pairKey;
    std::vector<float> lambdas;
    while (i < rows.size() && rows[i].pairKey == pk) {
      lambdas.push_back(rows[i].lambda);
      ++i;
    }
    collisionLambdaWarmStart[pk] = std::move(lambdas);
  }
}

void XPBDSolver::applyCollisionVelocityResponse(
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies,
    const std::vector<CollisionContact>& contacts,
    float deltatime) const {
  constexpr float kVelocityEpsilon = 1e-6f;
  constexpr float kActiveContactSlop = 2e-4f;
  constexpr float kRelaxation = 1.0f;
  constexpr float kTangentEps = 1e-5f;
  // When v·n ≈ 0 (resting contact), still allow a Coulomb cone using a small impulse floor so
  // tangential creep from gravity does not build up wobble in stacks.
  constexpr float kRestingFrictionImpulseFloor = 30.0f;

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
    const float signedDistance = glm::dot(contact.pointA - contact.pointB, normal);
    if (signedDistance > kActiveContactSlop) {
      continue;
    }

    const glm::vec3 centerA = bodyA->getWorldCenterOfMass();
    const glm::vec3 centerB = bodyB->getWorldCenterOfMass();
    const glm::vec3 offsetA = contact.pointA - centerA;
    const glm::vec3 offsetB = contact.pointB - centerB;
    const float normalMass = invMassA + invMassB +
        angularEffectiveMass(*bodyA, offsetA, normal) +
        angularEffectiveMass(*bodyB, offsetB, normal);
    if (normalMass <= kVelocityEpsilon) {
      continue;
    }

    glm::vec3 relativeVelocity =
        velocityAtPoint(*bodyA, offsetA) - velocityAtPoint(*bodyB, offsetB);
    const float normalSpeed = glm::dot(relativeVelocity, normal);

    glm::vec3 normalImpulse(0.0f);
    if (normalSpeed < -kVelocityEpsilon) {
      const float normalImpulseMagnitude = -normalSpeed / normalMass;
      normalImpulse = kRelaxation * normalImpulseMagnitude * normal;
      applyImpulse(*bodyA, normalImpulse, offsetA);
      applyImpulse(*bodyB, -normalImpulse, offsetB);
    }

    float frictionCoefficient = kRigidContactStaticFrictionMu;
    if (dragBody && (bodyA == dragBody || bodyB == dragBody)) {
      frictionCoefficient *= std::clamp(dragContactFrictionScale, 0.0f, 1.0f);
    }

    const float jN = glm::length(normalImpulse);
    const float frictionNormalRef = std::max(jN, kRestingFrictionImpulseFloor * deltatime);
    const float jMax = frictionCoefficient * frictionNormalRef;

    // Two passes in the tangent plane: one axis rarely zeros full slip when |Jt| is clamped.
    for (int frictionPass = 0; frictionPass < 2; ++frictionPass) {
      relativeVelocity = velocityAtPoint(*bodyA, offsetA) - velocityAtPoint(*bodyB, offsetB);
      glm::vec3 tangential =
          relativeVelocity - normal * glm::dot(relativeVelocity, normal);
      const float tangentialLenSq = glm::dot(tangential, tangential);
      if (tangentialLenSq < kTangentEps * kTangentEps) {
        break;
      }

      const glm::vec3 tangentDir = tangential * (1.0f / std::sqrt(tangentialLenSq));
      const float tangentMass = invMassA + invMassB +
          angularEffectiveMass(*bodyA, offsetA, tangentDir) +
          angularEffectiveMass(*bodyB, offsetB, tangentDir);
      if (tangentMass <= kVelocityEpsilon) {
        break;
      }

      float jt = -glm::dot(relativeVelocity, tangentDir) / tangentMass;
      jt = std::clamp(jt, -jMax, jMax);
      const glm::vec3 frictionImpulse = jt * tangentDir;
      applyImpulse(*bodyA, frictionImpulse, offsetA);
      applyImpulse(*bodyB, -frictionImpulse, offsetB);
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

  for (int substep = 0; substep < substeps; ++substep) {
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
    applyProjectedRigidState(
        buffers.vertices,
        buffers.rigidBodies,
        buffers.previousCentersOfMass,
        buffers.previousOrientations,
        h);

    const auto substepContacts = collectCollisionContacts(simulatedBodies);
    applyCollisionVelocityResponse(simulatedBodies, substepContacts, h);
  }

  captureCollisionLambdaWarmStart(simulatedBodies, constraints);

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
