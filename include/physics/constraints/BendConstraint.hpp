#pragma once

#include <array>
#include <cstdint>
#include <limits>

namespace physics {

struct BendConstraint {
  BendConstraint() = default;

  BendConstraint(uint32_t sharedEdgeIndex,
                 uint32_t sharedEdgeParticleA,
                 uint32_t sharedEdgeParticleB,
                 uint32_t oppositeParticleA,
                 uint32_t oppositeParticleB,
                 float restAngle,
                 uint32_t triangleA = std::numeric_limits<uint32_t>::max(),
                 uint32_t triangleB = std::numeric_limits<uint32_t>::max(),
                 float compliance = 0.0f,
                 float lambda = 0.0f)
      : sharedEdgeIndex(sharedEdgeIndex),
        sharedEdgeParticleIndices { sharedEdgeParticleA, sharedEdgeParticleB },
        oppositeParticleIndices { oppositeParticleA, oppositeParticleB },
        triangleIndices { triangleA, triangleB },
        restAngle(restAngle),
        compliance(compliance),
        lambda_(lambda) {}

  void resetLambda() { lambda_ = 0.0f; }
  float getLambda() const { return lambda_; }
  void setLambda(float lambda) { lambda_ = lambda; }

  uint32_t sharedEdgeIndex = std::numeric_limits<uint32_t>::max();
  std::array<uint32_t, 2> sharedEdgeParticleIndices { 0, 0 };
  std::array<uint32_t, 2> oppositeParticleIndices { 0, 0 };
  std::array<uint32_t, 2> triangleIndices {
      std::numeric_limits<uint32_t>::max(),
      std::numeric_limits<uint32_t>::max(),
  };
  float restAngle = 0.0f;
  float compliance = 0.0f;

private:
  float lambda_ = 0.0f;
};

} // namespace physics
