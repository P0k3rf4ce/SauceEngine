#include <physics/XPBD.hpp>
#include <physics/Cloth.hpp>
#include <physics/SphereCollider.hpp>
#include <physics/ContactInfo.hpp>
#include <physics/Vertex.hpp>
#include <physics/constraints/Constraint.hpp>
#include <physics/constraints/CollisionConstraint.hpp>

#include <app/Entity.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

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

void projectStretch(std::vector<ClothParticle>& particles, StretchConstraint& c) {
  const uint32_t i0 = c.particleIndices[0];
  const uint32_t i1 = c.particleIndices[1];
  
  if (i0 >= particles.size() || i1 >= particles.size()) {
    return;
  }
  
  glm::vec3& x0 = particles[i0].predictedPosition;
  glm::vec3& x1 = particles[i1].predictedPosition;
  glm::vec3 delta = x1 - x0;

  float currentDist = glm::length(delta);
  // Avoid division by zero
  if (currentDist < 1e-6f) {
    return;
  }
  float constraint = currentDist - c.restLength;
 
  x0 += (delta/currentDist) * (constraint/2.f);
  x1 -= (delta/currentDist) * (constraint/2.f);
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

void XPBDSolver::solvePositions(
    std::vector<sauce::RigidBodyComponent>& rigidBodies,
    std::vector<std::unique_ptr<Constraint>>& constraints,
    float deltatime) {
  /*
   * adapted from https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
   */
  glm::vec3 velocity;
  float w = 0.0f;
  std::vector<physics::Vertex> centers;

  centers.reserve(rigidBodies.size());
  for (auto& rigidBody : rigidBodies) {
    auto* owner = rigidBody.getOwner();
    auto* meshRenderer = owner ? owner->getComponent<sauce::MeshRendererComponent>() : nullptr;
    if (!meshRenderer || !meshRenderer->getMesh()) {
      continue;
    }

    w = rigidBody.getInvMass();
    velocity = rigidBody.getVelocity() + deltatime * (w * rigidBody.getExternalForces());
    rigidBody.setPosition(rigidBody.getPosition() + velocity * deltatime);

    centers.push_back({
        rigidBody.getCenterOfMass(),
        glm::vec3(0.0f),
        rigidBody.getInvMass(),
    });
  }

  constraints = generateCollisionConstraints(rigidBodies);
  projectConstraints(centers, constraints, deltatime);
}

void XPBDSolver::projectConstraints(
    std::vector<physics::Vertex>& vertices,
    std::vector<std::unique_ptr<Constraint>>& constraints,
    float deltatime) {
  if (vertices.empty() || constraints.empty()) {
    return;
  }

  for (int iter = 0; iter < solverIterations; ++iter) {
    if (iter == 0) {
      for (auto& constraint : constraints) {
        constraint->resetLambda();
      }
    }

    for (auto& constraint : constraints) {
      constraint->solve(vertices, deltatime);
    }
  }
}

std::vector<std::unique_ptr<Constraint>> XPBDSolver::generateCollisionConstraints(
    std::vector<sauce::RigidBodyComponent>& rigidBodies
) {
    std::vector<std::unique_ptr<Constraint>> constraints;

    // Build a bounding sphere for each rigid body from its mesh vertices.
    struct BodySphere {
        uint32_t index;
        SphereCollider sphere;
    };

    std::vector<BodySphere> bodies;
    bodies.reserve(rigidBodies.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(rigidBodies.size()); ++i) {
        auto& rb = rigidBodies[i];

        auto* owner = rb.getOwner();
        if (!owner) continue;

        auto* meshComp = owner->getComponent<sauce::MeshRendererComponent>();
        if (!meshComp || !meshComp->getMesh()) continue;

        const auto& verts = meshComp->getMesh()->getVertices();
        if (verts.empty()) continue;

        glm::vec3 center = rb.getPosition();

        float maxRadiusSq = 0.0f;
        for (const auto& v : verts) {
            float dSq = glm::length2(v.position - center);
            if (dSq > maxRadiusSq) {
                maxRadiusSq = dSq;
            }
        }

        SphereCollider sphere;
        sphere.center = center;
        sphere.radius = std::sqrt(maxRadiusSq);

        bodies.push_back({ i, sphere });
    }

    // Pairwise collision detection using SphereCollider::checkCollision.
    // For every contact produced, emit a CollisionConstraint.
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            std::vector<ContactInfo> contacts;

            if (!bodies[i].sphere.checkCollision(bodies[j].sphere, contacts)) {
                continue;
            }

            for (const auto& c : contacts) {
                constraints.push_back(std::make_unique<CollisionConstraint>(
                    bodies[i].index,
                    bodies[j].index,
                    c.contactNormal,
                    c.depth,
                    0.0f // zero compliance = perfectly rigid contact
                ));
            }
        }
    }

    return constraints;
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
      for (auto& c : cloth.stretchConstraints) {
        projectStretch(particles, c);
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
