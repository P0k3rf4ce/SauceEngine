#pragma once

#include <physics/constraints/Constraint.hpp>
#include <glm/glm.hpp>
#include <cmath>

namespace physics {

struct BendConstraint : public Constraint {

  BendConstraint() = default;

  BendConstraint(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3,
                 float rest, float comp = 0.0f)
      : Constraint(comp),
        i0(i0), i1(i1), i2(i2), i3(i3),
        restAngle(rest) {}

  void solve(std::vector<physics::Vertex>& vertices, float dt) override {

    if (i0 >= vertices.size() || i1 >= vertices.size() ||
        i2 >= vertices.size() || i3 >= vertices.size()) return;

    auto& v0 = vertices[i0];
    auto& v1 = vertices[i1];
    auto& v2 = vertices[i2];
    auto& v3 = vertices[i3];

    float w0 = v0.invMass;
    float w1 = v1.invMass;
    float w2 = v2.invMass;
    float w3 = v3.invMass;

    if (w0 + w1 + w2 + w3 <= 0.0f) return;

    glm::vec3 e = v3.position - v2.position;

    glm::vec3 n1 = glm::normalize(glm::cross(v2.position - v0.position,
                                             v3.position - v0.position));
    glm::vec3 n2 = glm::normalize(glm::cross(v3.position - v1.position,
                                             v2.position - v1.position));

    float dot = glm::clamp(glm::dot(n1, n2), -1.0f, 1.0f);

    float phi = std::acos(dot);

    float C = phi - restAngle;

    float alphaTilde = compliance / (dt * dt);

    glm::vec3 q0 = (glm::cross(e, n1) / glm::length(glm::cross(e, n1)));
    glm::vec3 q1 = (glm::cross(e, n2) / glm::length(glm::cross(e, n2)));
    glm::vec3 q2 = -q0;
    glm::vec3 q3 = -q1;

    float denom =
        w0 * glm::dot(q0, q0) +
        w1 * glm::dot(q1, q1) +
        w2 * glm::dot(q2, q2) +
        w3 * glm::dot(q3, q3) +
        alphaTilde;

    if (denom <= 0.0f) return;

    float deltaLambda = (-C - alphaTilde * lambda) / denom;

    v0.position += (w0 * deltaLambda) * q0;
    v1.position += (w1 * deltaLambda) * q1;
    v2.position += (w2 * deltaLambda) * q2;
    v3.position += (w3 * deltaLambda) * q3;

    lambda += deltaLambda;
  }

  uint32_t i0, i1, i2, i3;
  float restAngle = 0.0f;
};

}
