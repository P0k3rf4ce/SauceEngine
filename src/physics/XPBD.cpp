#include <physics/XPBD.hpp>
#include <physics/Cloth.hpp>
#include <physics/Vertex.hpp>
#include <physics/constraints/CollisionConstraint.hpp>

#include <algorithm>
#include <cmath>

namespace physics {

namespace {

constexpr float kBendEps = 1e-10f;

void resetClothLambdas(ClothData& cloth) {
  for (auto& c : cloth.stretchConstraints) {
    c.resetLambda();
  }
  for (auto& c : cloth.bendConstraints) {
    c.resetLambda();
  }
}

void projectBend(std::vector<ClothParticle>& particles, BendConstraint& c, float h) {
  const uint32_t i0 = c.oppositeParticleIndices[0];
  const uint32_t i1 = c.sharedEdgeParticleIndices[0];
  const uint32_t i2 = c.sharedEdgeParticleIndices[1];
  const uint32_t i3 = c.oppositeParticleIndices[1];

  if (i0 >= particles.size() || i1 >= particles.size() || i2 >= particles.size() ||
      i3 >= particles.size()) {
    return;
  }

  glm::vec3& x0 = particles[i0].predictedPosition;
  glm::vec3& x1 = particles[i1].predictedPosition;
  glm::vec3& x2 = particles[i2].predictedPosition;
  glm::vec3& x3 = particles[i3].predictedPosition;

  const float w0 = particles[i0].isStatic() ? 0.0f : particles[i0].invMass;
  const float w1 = particles[i1].isStatic() ? 0.0f : particles[i1].invMass;
  const float w2 = particles[i2].isStatic() ? 0.0f : particles[i2].invMass;
  const float w3 = particles[i3].isStatic() ? 0.0f : particles[i3].invMass;

  const glm::vec3 e1 = x1 - x0;
  const glm::vec3 e2 = x2 - x0;
  const glm::vec3 f1 = x2 - x3;
  const glm::vec3 f2 = x1 - x3;

  glm::vec3 A = glm::cross(e1, e2);
  glm::vec3 B = glm::cross(f1, f2);

  const float lenA = glm::length(A);
  const float lenB = glm::length(B);
  if (lenA < kBendEps || lenB < kBendEps) {
    return;
  }

  const glm::vec3 na = A / lenA;
  const glm::vec3 nb = B / lenB;

  const float cosRest = std::cos(c.restAngle);
  const float C = glm::dot(na, nb) - cosRest;

  const glm::vec3 wA = (nb - na * glm::dot(na, nb)) / lenA;
  const glm::vec3 wBvec = (na - nb * glm::dot(na, nb)) / lenB;

  const glm::vec3 g0 = glm::cross(wA, e2 - e1);
  const glm::vec3 g1 = glm::cross(e2, wA) + glm::cross(wBvec, f1);
  const glm::vec3 g2 = glm::cross(wA, e1) + glm::cross(f2, wBvec);
  const glm::vec3 g3 = glm::cross(f1 - f2, wBvec);

  float denom = 0.0f;
  if (w0 > 0.0f) {
    denom += w0 * glm::dot(g0, g0);
  }
  if (w1 > 0.0f) {
    denom += w1 * glm::dot(g1, g1);
  }
  if (w2 > 0.0f) {
    denom += w2 * glm::dot(g2, g2);
  }
  if (w3 > 0.0f) {
    denom += w3 * glm::dot(g3, g3);
  }

  const float alphaTilde = c.compliance / (h * h);
  denom += alphaTilde;
  if (denom < 1e-14f) {
    return;
  }

  float lambda = c.getLambda();
  const float deltaLambda = (-C - alphaTilde * lambda) / denom;
  c.setLambda(lambda + deltaLambda);

  if (w0 > 0.0f) {
    x0 += w0 * deltaLambda * g0;
  }
  if (w1 > 0.0f) {
    x1 += w1 * deltaLambda * g1;
  }
  if (w2 > 0.0f) {
    x2 += w2 * deltaLambda * g2;
  }
  if (w3 > 0.0f) {
    x3 += w3 * deltaLambda * g3;
  }
}

} // namespace

void XPBDSolver::projectConstraints(
    std::vector<physics::Vertex>& vertices,
    std::vector<CollisionConstraint>& constraints,
    float deltatime) {
  if (vertices.empty() || constraints.empty()) {
    return;
  }

  for (int iter = 0; iter < solverIterations; ++iter) {
    if (iter == 0) {
      for (auto& constraint : constraints) {
        constraint.resetLambda();
      }
    }

    for (auto& constraint : constraints) {
      constraint.solve(vertices, deltatime);
    }
  }
}

void XPBDSolver::solveCloth(ClothData& cloth, float deltatime, const glm::vec3& externalAcceleration) {
  if (cloth.empty() || deltatime <= 0.0f) {
    return;
  }

  const int substeps = std::max(1, clothSubsteps);
  const float h = deltatime / static_cast<float>(substeps);

  auto& particles = cloth.particles;

  for (int s = 0; s < substeps; ++s) {
    for (auto& p : particles) {
      p.previousPosition = p.position;
      if (p.isStatic()) {
        p.velocity = glm::vec3(0.0f);
        p.predictedPosition = p.position;
        continue;
      }

      p.velocity += externalAcceleration * h;
      p.predictedPosition = p.position + p.velocity * h;
    }

    resetClothLambdas(cloth);

    for (int iter = 0; iter < solverIterations; ++iter) {
      for (auto& c : cloth.bendConstraints) {
        projectBend(particles, c, h);
      }
    }

    for (auto& p : particles) {
      if (p.isStatic()) {
        p.predictedPosition = p.position;
        p.velocity = glm::vec3(0.0f);
        continue;
      }

      p.position = p.predictedPosition;
      p.velocity = (p.position - p.previousPosition) / h;
    }
  }
}

} // namespace physics
