#pragma once

#include <cstdint>
#include <vector>

namespace sauce {

struct ClothSettings {
  float defaultInvMass = 1.0f;
  float stretchCompliance = 0.0f;
  float bendCompliance = 0.0f;
  float damping = 0.0f;
  float gravityScale = 1.0f;
  int solverSubsteps = 4;
  std::vector<uint32_t> pinnedParticleIndices;
};

} // namespace sauce
