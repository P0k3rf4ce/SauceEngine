#pragma once

#include <glm/glm.hpp>

#include <app/components/RigidBodyComponent.hpp>

#include <cmath>
#include <cstdint>
#include <tuple>
#include <utility>

namespace physics::detail {

inline std::pair<uintptr_t, uintptr_t> canonicalRigidPair(
    const sauce::RigidBodyComponent* a,
    const sauce::RigidBodyComponent* b) {
  uintptr_t pa = reinterpret_cast<uintptr_t>(a);
  uintptr_t pb = reinterpret_cast<uintptr_t>(b);
  if (pa > pb) {
    std::swap(pa, pb);
  }
  return {pa, pb};
}

inline std::tuple<int, int, int> quantizedWorldMid(const glm::vec3& mid) {
  auto q = [](float x) { return static_cast<int>(std::lround(x * 1000.0f)); };
  return {q(mid.x), q(mid.y), q(mid.z)};
}

} // namespace physics::detail
