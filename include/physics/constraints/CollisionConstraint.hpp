#pragma once

#include <physics/constraints/Constraint.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <limits>

namespace physics {

// Position Based Dynamics Collision constraint
struct CollisionConstraint : public Constraint {
  CollisionConstraint() = default;

  // Construct from two vertex indices plus collision geometry.
  CollisionConstraint(uint32_t a, uint32_t b, glm::vec3 normal, float depth,
                      float comp = 0.0f)
      : Constraint(comp), indexA(a), indexB(b), contactNormal(normal), penetrationDepth(depth) {}

  CollisionConstraint(uint32_t a, uint32_t b, glm::vec3 normal, float depth,
                      glm::vec3 offsetA, glm::vec3 offsetB, float comp = 0.0f)
      : Constraint(comp), indexA(a), indexB(b), contactNormal(normal), penetrationDepth(depth),
        localOffsetA(offsetA), localOffsetB(offsetB), useContactOffsets(true) {}

  // Construct from single vertex colliding with a static surface.
  CollisionConstraint(uint32_t a, glm::vec3 contactPt, glm::vec3 normal,
                      float comp = 0.0f)
      : Constraint(comp), indexA(a), indexB(UINT32_MAX), contactPoint(contactPt),
        contactNormal(normal), isStaticCollision(true) {}

  void solve(std::vector<physics::Vertex>& vertices, float deltatime) override {
    if (isStaticCollision) {
      solveStatic(vertices, deltatime);
    } else {
      solveDynamic(vertices, deltatime);
    }
  }

  uint32_t indexA = 0;
  uint32_t indexB = 0;
  glm::vec3 contactPoint = glm::vec3(0.0f);
  glm::vec3 contactNormal = glm::vec3(0.0f, 1.0f, 0.0f);
  float penetrationDepth = 0.0f;
  glm::vec3 localOffsetA = glm::vec3(0.0f);
  glm::vec3 localOffsetB = glm::vec3(0.0f);
  bool isStaticCollision = false;
  bool useContactOffsets = false;

private:
  static glm::mat3 worldInvInertiaTensor(const physics::Vertex& v) {
    const glm::mat3 rotation = glm::mat3_cast(v.orientation);
    return rotation * v.invInertiaTensor * glm::transpose(rotation);
  }

  // Dynamic collision, vertex to vertex
  void solveDynamic(std::vector<physics::Vertex>& vertices, float deltatime) {
    if (indexA >= vertices.size() || indexB >= vertices.size()) return;

    physics::Vertex& va = vertices[indexA];
    physics::Vertex& vb = vertices[indexB];

    const float w1 = va.invMass;
    const float w2 = vb.invMass;
    if (w1 + w2 <= 1e-8f) return;

    const glm::vec3 worldOffsetA = useContactOffsets ? va.orientation * localOffsetA
                                                     : glm::vec3(0.0f);
    const glm::vec3 worldOffsetB = useContactOffsets ? vb.orientation * localOffsetB
                                                     : glm::vec3(0.0f);
    const glm::vec3 pointA = va.position + worldOffsetA;
    const glm::vec3 pointB = vb.position + worldOffsetB;

    const float C = glm::dot(pointA - pointB, contactNormal) - penetrationDepth;

    if (C >= -1e-8f) return;

    const float alphaTilde = compliance / (deltatime * deltatime);
    const glm::vec3 angularA = glm::cross(worldOffsetA, contactNormal);
    const glm::vec3 angularB = glm::cross(worldOffsetB, contactNormal);
    const glm::mat3 worldInvInertiaA = worldInvInertiaTensor(va);
    const glm::mat3 worldInvInertiaB = worldInvInertiaTensor(vb);
    const float angularTermA = useContactOffsets
        ? glm::dot(glm::cross(worldInvInertiaA * angularA, worldOffsetA), contactNormal)
        : 0.0f;
    const float angularTermB = useContactOffsets
        ? glm::dot(glm::cross(worldInvInertiaB * angularB, worldOffsetB), contactNormal)
        : 0.0f;

    const float denom = w1 + w2 + angularTermA + angularTermB + alphaTilde;
    if (denom <= 1e-8f) return;

    const float deltaLambda = (-C - alphaTilde * lambda) / denom;

    const glm::vec3 impulse = deltaLambda * contactNormal;
    const glm::vec3 deltaP_a = w1 * impulse;
    const glm::vec3 deltaP_b = w2 * impulse;

    va.position += deltaP_a;
    vb.position -= deltaP_b;

    const glm::vec3 dOmega_a = worldInvInertiaA * glm::cross(worldOffsetA, impulse);
    const glm::vec3 dOmega_b = worldInvInertiaB * glm::cross(worldOffsetB, -impulse);
    va.orientation = glm::normalize(va.orientation + 0.5f * glm::quat(0.0f, dOmega_a) * va.orientation);
    vb.orientation = glm::normalize(vb.orientation + 0.5f * glm::quat(0.0f, dOmega_b) * vb.orientation);

    lambda += deltaLambda;
  }

  // Static collision, a single vertex against a fixed surface point
  void solveStatic(std::vector<physics::Vertex>& vertices, float deltatime) {
    if (indexA >= vertices.size()) return;

    physics::Vertex& va = vertices[indexA];
    const float w = va.invMass;
    if (w <= 1e-8f) return;

    const float C = glm::dot(va.position - contactPoint, contactNormal);

    if (C >= -1e-8f) return;

    const float alphaTilde = compliance / (deltatime * deltatime);

    const float denom = w + alphaTilde;
    const float deltaLambda = (-C - alphaTilde * lambda) / denom;

    const glm::vec3 deltaP = (w * deltaLambda) * contactNormal;
    va.position += deltaP;

    // Orientation correction
    const glm::vec3 r = contactPoint - va.position;
    const glm::vec3 dOmega = worldInvInertiaTensor(va) * glm::cross(r, deltaP);
    va.orientation = glm::normalize(va.orientation + 0.5f * glm::quat(0.0f, dOmega) * va.orientation);

    lambda += deltaLambda;
  }
};

}
