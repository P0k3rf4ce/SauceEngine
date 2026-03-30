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
  std::vector<glm::vec3> previousCenters;
  std::vector<glm::quat> previousOrientations;
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
  buffers.previousCenters.clear();
  buffers.previousOrientations.clear();

  buffers.vertices.reserve(rigidBodies.size());
  buffers.rigidBodies.reserve(rigidBodies.size());
  buffers.previousCenters.reserve(rigidBodies.size());
  buffers.previousOrientations.reserve(rigidBodies.size());

  for (auto* rigidBody : rigidBodies) {
    buffers.previousCenters.push_back(rigidBody->getWorldCenterOfMass());
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

} // namespace

void XPBDSolver::clearStaticContactState() {
  lastStaticContactNormals.clear();
  lastStaticContactCounts.clear();
}

void XPBDSolver::resetStaticContactState(size_t bodyCount) {
  lastStaticContactNormals.clear();
  lastStaticContactCounts.clear();
  lastStaticContactNormals.resize(bodyCount);
  lastStaticContactCounts.resize(bodyCount, 0);
}

void XPBDSolver::recordStaticContact(uint32_t bodyIndex, const glm::vec3& normal) {
  if (bodyIndex >= lastStaticContactNormals.size()) {
    return;
  }

  lastStaticContactNormals[bodyIndex].push_back(normal);
  ++lastStaticContactCounts[bodyIndex];
}

bool XPBDSolver::hasStaticContacts(size_t bodyIndex) const {
  return bodyIndex < lastStaticContactCounts.size() && lastStaticContactCounts[bodyIndex] > 0;
}

const std::vector<glm::vec3>& XPBDSolver::staticContactNormals(size_t bodyIndex) const {
  static const std::vector<glm::vec3> emptyNormals;
  if (bodyIndex >= lastStaticContactNormals.size()) {
    return emptyNormals;
  }

  return lastStaticContactNormals[bodyIndex];
}

void XPBDSolver::stabilizePostSolveVelocities(size_t bodyIndex,
                                              glm::vec3& linearVelocity,
                                              glm::vec3& angularVelocity) const {
  if (!hasStaticContacts(bodyIndex)) {
    return;
  }

  glm::vec3 accumulatedNormal(0.0f);
  for (const auto& normal : staticContactNormals(bodyIndex)) {
    const float normalLenSq = glm::dot(normal, normal);
    if (normalLenSq <= 1e-10f) {
      continue;
    }

    const glm::vec3 n = normal / std::sqrt(normalLenSq);
    accumulatedNormal += n;
    const float closingSpeed = glm::dot(linearVelocity, n);
    if (closingSpeed < 0.0f) {
      linearVelocity -= closingSpeed * n;
    }
  }

  constexpr float kTangentialVelocityRetention = 0.995f;
  constexpr float kAngularVelocityRetention = 0.995f;
  constexpr float kRestLinearSpeed = 0.01f;
  constexpr float kRestAngularSpeed = 0.05f;

  const float accumulatedNormalLenSq = glm::dot(accumulatedNormal, accumulatedNormal);
  if (accumulatedNormalLenSq > 1e-10f) {
    const glm::vec3 contactNormal = accumulatedNormal / std::sqrt(accumulatedNormalLenSq);
    const float normalSpeed = glm::dot(linearVelocity, contactNormal);
    const glm::vec3 tangentialVelocity = linearVelocity - normalSpeed * contactNormal;
    linearVelocity = normalSpeed * contactNormal + tangentialVelocity * kTangentialVelocityRetention;
  }

  angularVelocity *= kAngularVelocityRetention;

  if (glm::length2(linearVelocity) < kRestLinearSpeed * kRestLinearSpeed &&
      glm::length2(angularVelocity) < kRestAngularSpeed * kRestAngularSpeed) {
    linearVelocity = glm::vec3(0.0f);
    angularVelocity = glm::vec3(0.0f);
  }
}

void XPBDSolver::solvePositions(
    std::vector<sauce::RigidBodyComponent*>& rigidBodies,
    std::vector<std::unique_ptr<Constraint>>& constraints,
    float deltatime) {
  /*
   * adapted from https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
   */
  if (deltatime <= 0.0f) {
    return;
  }

  auto simulatedBodies = collectSimulatedRigidBodies(rigidBodies);
  if (simulatedBodies.empty()) {
    constraints.clear();
    clearStaticContactState();
    return;
  }

  std::vector<glm::vec3> initialCenters;
  std::vector<glm::quat> initialOrientations;
  initialCenters.reserve(simulatedBodies.size());
  initialOrientations.reserve(simulatedBodies.size());
  for (auto* rigidBody : simulatedBodies) {
    initialCenters.push_back(rigidBody->getWorldCenterOfMass());
    initialOrientations.push_back(rigidBody->getOrientation());
  }

  const int substeps = std::max(1, rigidSubsteps);
  const float h = deltatime / static_cast<float>(substeps);

  RigidSolveBuffers buffers;
  for (int substep = 0; substep < substeps; ++substep) {
    resetStaticContactState(simulatedBodies.size());
    initializeRigidSolveBuffers(simulatedBodies, h, buffers);
    constraints = generateCollisionConstraints(buffers.rigidBodies);
    projectConstraints(buffers.vertices, constraints, h);
    applyProjectedRigidState(buffers.vertices, buffers.rigidBodies);
  }

  for (size_t i = 0; i < simulatedBodies.size(); ++i) {
    auto* rigidBody = simulatedBodies[i];
    if (!rigidBody) {
      continue;
    }

    if (rigidBody->getInvMass() <= 1e-8f || deltatime <= 0.0f) {
      rigidBody->setVelocity(glm::vec3(0.0f));
      rigidBody->setAngularVelocity(glm::vec3(0.0f));
      continue;
    }

    glm::vec3 velocity = (rigidBody->getWorldCenterOfMass() - initialCenters[i]) / deltatime;
    glm::vec3 angularVelocity = reconstructAngularVelocity(
        initialOrientations[i],
        rigidBody->getOrientation(),
        deltatime);
    stabilizePostSolveVelocities(i, velocity, angularVelocity);

    rigidBody->setVelocity(velocity);
    rigidBody->setAngularVelocity(angularVelocity);
  }
}

} // namespace physics
