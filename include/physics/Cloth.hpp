#pragma once

#include <app/modeling/Mesh.hpp>

#include <physics/constraints/BendConstraint.hpp>
#include <physics/constraints/StretchConstraint.hpp>

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace physics {

inline constexpr uint32_t kInvalidClothIndex = std::numeric_limits<uint32_t>::max();

struct ClothParticle {
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 previousPosition = glm::vec3(0.0f);
  glm::vec3 predictedPosition = glm::vec3(0.0f);
  glm::vec3 velocity = glm::vec3(0.0f);
  float invMass = 1.0f;
  bool pinned = false;

  bool isStatic() const { return pinned || invMass <= 0.0f; }
};

struct ClothEdge {
  std::array<uint32_t, 2> particleIndices { 0, 0 };
  std::array<uint32_t, 2> adjacentTriangleIndices { kInvalidClothIndex, kInvalidClothIndex };

  bool isBoundary() const {
    return adjacentTriangleIndices[0] == kInvalidClothIndex ||
           adjacentTriangleIndices[1] == kInvalidClothIndex;
  }
};

struct ClothTopology {
  std::vector<uint32_t> particleIndices;
  std::vector<uint32_t> triangleIndices;
  std::vector<ClothEdge> edges;

  size_t triangleCount() const { return triangleIndices.size() / 3; }
};

struct ClothData {
  std::vector<ClothParticle> particles;
  ClothTopology topology;
  std::vector<StretchConstraint> stretchConstraints;
  std::vector<BendConstraint> bendConstraints;
  std::string debugName;

  bool empty() const { return particles.empty(); }

  size_t pinnedParticleCount() const;
  size_t staticParticleCount() const;
};

std::optional<ClothData> buildClothDataFromMesh(
    const sauce::modeling::Mesh& mesh,
    const std::string& debugName = {},
    float defaultInvMass = 1.0f);

} // namespace physics
