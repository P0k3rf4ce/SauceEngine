#pragma once

#include <physics/constraints/Constraint.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>

namespace physics {

/// Static friction coefficient for rigid contacts (position projection + velocity impulses).
/// >1 is common for game stacks (rough wood / high grip); keeps Jenga-style blocks from skating.
inline constexpr float kRigidContactStaticFrictionMu = 1.35f;

namespace {
constexpr float kCollisionMassEpsilon = 1e-8f;
// Ignore penetration shallower than this (m) to stop GS + velocity fighting on noise.
constexpr float kCollisionPenetrationSlop = 1.0e-4f;
constexpr float kNormalCorrectionRelax = 0.92f;
constexpr float kTangentCorrectionRelax = 0.97f;
constexpr float kStaticFrictionCoefficient = kRigidContactStaticFrictionMu;
}

struct CollisionConstraint : public Constraint {
  CollisionConstraint() = default;

  CollisionConstraint(uint32_t a, uint32_t b, glm::vec3 normal,
                      glm::vec3 offsetA, glm::vec3 offsetB, float comp = 0.0f,
                      glm::vec3 warmMid = glm::vec3(0.0f), float warmLambda = 0.0f,
                      float frictionCoeff = kRigidContactStaticFrictionMu)
      : Constraint(comp), indexA(a), indexB(b), contactNormal(normal),
        localOffsetA(offsetA), localOffsetB(offsetB), warmSortMid(warmMid),
        frictionCoefficient(frictionCoeff), warmStartLambda(warmLambda) {}

  void resetLambda() override { lambda = warmStartLambda; }

  void solve(std::vector<physics::Vertex>& vertices, float deltatime) override {
    if (indexA >= vertices.size() || indexB >= vertices.size()) return;

    physics::Vertex& va = vertices[indexA];
    physics::Vertex& vb = vertices[indexB];

    const float w1 = va.invMass;
    const float w2 = vb.invMass;
    if (w1 + w2 <= kCollisionMassEpsilon) return;

    const glm::vec3 worldOffsetA = va.orientation * localOffsetA;
    const glm::vec3 worldOffsetB = vb.orientation * localOffsetB;
    const glm::vec3 pointA = va.position + worldOffsetA;
    const glm::vec3 pointB = vb.position + worldOffsetB;

    const float C = glm::dot(pointA - pointB, contactNormal);
    if (C >= -kCollisionPenetrationSlop) return;

    const float alphaTilde = compliance / (deltatime * deltatime);
    const glm::mat3 worldInvInertiaA = worldInvInertiaTensor(va);
    const glm::mat3 worldInvInertiaB = worldInvInertiaTensor(vb);

    const float normalInvMass =
        computeGeneralizedInverseMass(va, worldOffsetA, contactNormal, worldInvInertiaA) +
        computeGeneralizedInverseMass(vb, worldOffsetB, contactNormal, worldInvInertiaB);

    const float denom = normalInvMass + alphaTilde;
    if (denom <= kCollisionMassEpsilon) return;

    const float rawDeltaLambda = (-C - alphaTilde * lambda) / denom;
    const float appliedDeltaLambda = rawDeltaLambda * kNormalCorrectionRelax;
    lambda += appliedDeltaLambda;

    // Snapshot contact point positions before normal correction
    const glm::vec3 prePointA = pointA;
    const glm::vec3 prePointB = pointB;

    // Apply normal correction
    const glm::vec3 normalImpulse = appliedDeltaLambda * contactNormal;
    va.position += w1 * normalImpulse;
    vb.position -= w2 * normalImpulse;

    const glm::vec3 dOmegaA = worldInvInertiaA * glm::cross(worldOffsetA, normalImpulse);
    const glm::vec3 dOmegaB = worldInvInertiaB * glm::cross(worldOffsetB, -normalImpulse);
    va.orientation = glm::normalize(va.orientation + 0.5f * glm::quat(0.0f, dOmegaA) * va.orientation);
    vb.orientation = glm::normalize(vb.orientation + 0.5f * glm::quat(0.0f, dOmegaB) * vb.orientation);

    const glm::vec3 postPointA = va.position + va.orientation * localOffsetA;
    const glm::vec3 postPointB = vb.position + vb.orientation * localOffsetB;
    const glm::vec3 relativeSlide = (postPointA - prePointA) - (postPointB - prePointB);
    const glm::vec3 tangentialSlide = relativeSlide - glm::dot(relativeSlide, contactNormal) * contactNormal;
    const float tangentialSlideLenSq = glm::dot(tangentialSlide, tangentialSlide);

    const float normalCorrectionLen = std::abs(appliedDeltaLambda);
    const float frictionBudget = frictionCoefficient * normalCorrectionLen;
    if (tangentialSlideLenSq <= kCollisionMassEpsilon ||
        tangentialSlideLenSq <= frictionBudget * frictionBudget) {
      return;
    }

    const float tangentialSlideLen = std::sqrt(tangentialSlideLenSq);
    const glm::vec3 tangentDir = tangentialSlide / tangentialSlideLen;
    const float excessSlide = tangentialSlideLen - frictionBudget;

    const glm::vec3 postOffsetA = va.orientation * localOffsetA;
    const glm::vec3 postOffsetB = vb.orientation * localOffsetB;
    const float tangentInvMass =
        computeGeneralizedInverseMass(va, postOffsetA, tangentDir, worldInvInertiaA) +
        computeGeneralizedInverseMass(vb, postOffsetB, tangentDir, worldInvInertiaB);
    if (tangentInvMass <= kCollisionMassEpsilon) return;

    const float tangentLambda = (-excessSlide / tangentInvMass) * kTangentCorrectionRelax;
    const glm::vec3 tangentImpulse = tangentLambda * tangentDir;
    va.position += w1 * tangentImpulse;
    vb.position -= w2 * tangentImpulse;

    const glm::vec3 dOmegaTangentA = worldInvInertiaA * glm::cross(postOffsetA, tangentImpulse);
    const glm::vec3 dOmegaTangentB = worldInvInertiaB * glm::cross(postOffsetB, -tangentImpulse);
    va.orientation = glm::normalize(va.orientation + 0.5f * glm::quat(0.0f, dOmegaTangentA) * va.orientation);
    vb.orientation = glm::normalize(vb.orientation + 0.5f * glm::quat(0.0f, dOmegaTangentB) * vb.orientation);
  }

  uint32_t indexA = 0;
  uint32_t indexB = 0;
  glm::vec3 contactNormal = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 localOffsetA = glm::vec3(0.0f);
  glm::vec3 localOffsetB = glm::vec3(0.0f);
  /// World-space midpoint at constraint creation; stable sort key for warm-start matching.
  glm::vec3 warmSortMid = glm::vec3(0.0f);
  float frictionCoefficient = kStaticFrictionCoefficient;

private:
  float warmStartLambda = 0.0f;

  static glm::mat3 worldInvInertiaTensor(const physics::Vertex& v) {
    const glm::mat3 rotation = glm::mat3_cast(v.orientation);
    return rotation * v.invInertiaTensor * glm::transpose(rotation);
  }

  float computeGeneralizedInverseMass(const physics::Vertex& v,
                                      const glm::vec3& worldOffset,
                                      const glm::vec3& direction,
                                      const glm::mat3& worldInvInertia) const {
    const glm::vec3 angular = glm::cross(worldOffset, direction);
    return v.invMass + glm::dot(glm::cross(worldInvInertia * angular, worldOffset), direction);
  }
};

}
