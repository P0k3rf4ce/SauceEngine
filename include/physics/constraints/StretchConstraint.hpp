#pragma once

#include <array>
#include <cstdint>

namespace physics {

struct StretchConstraint {
  StretchConstraint() = default;

  StretchConstraint(uint32_t particleA, uint32_t particleB, float restLength,
                    float compliance = 0.0f, float lambda = 0.0f)
      : particleIndices { particleA, particleB },
        restLength(restLength),
        compliance(compliance),
        lambda_(lambda) {}

  void resetLambda() { lambda_ = 0.0f; }
  float getLambda() const { return lambda_; }
  void setLambda(float lambda) { lambda_ = lambda; }

  std::array<uint32_t, 2> particleIndices { 0, 0 };
  float restLength = 0.0f;
  float compliance = 0.0f;

private:
  float lambda_ = 0.0f;
};

} // namespace physics
