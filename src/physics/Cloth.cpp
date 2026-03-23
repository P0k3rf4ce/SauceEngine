#include <physics/Cloth.hpp>

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace physics {
namespace {

struct EdgeKey {
  uint32_t a = 0;
  uint32_t b = 0;

  bool operator==(const EdgeKey& other) const {
    return a == other.a && b == other.b;
  }
};

struct EdgeKeyHash {
  size_t operator()(const EdgeKey& key) const {
    return (static_cast<size_t>(key.a) << 32) ^ static_cast<size_t>(key.b);
  }
};

EdgeKey makeEdgeKey(uint32_t i0, uint32_t i1) {
  return { std::min(i0, i1), std::max(i0, i1) };
}

std::array<uint32_t, 3> getTriangle(
    const std::vector<uint32_t>& triangleIndices,
    uint32_t triangleIndex) {
  const size_t base = static_cast<size_t>(triangleIndex) * 3;
  return {
      triangleIndices[base + 0],
      triangleIndices[base + 1],
      triangleIndices[base + 2],
  };
}

uint32_t findOppositeParticle(
    const std::array<uint32_t, 3>& triangle,
    uint32_t edgeA,
    uint32_t edgeB) {
  for (uint32_t index : triangle) {
    if (index != edgeA && index != edgeB) {
      return index;
    }
  }

  return kInvalidClothIndex;
}

float computeRestBendAngle(
    const ClothData& clothData,
    uint32_t edgeA,
    uint32_t edgeB,
    uint32_t oppositeA,
    uint32_t oppositeB) {
  const glm::vec3& p0 = clothData.particles[oppositeA].position;
  const glm::vec3& p1 = clothData.particles[edgeA].position;
  const glm::vec3& p2 = clothData.particles[edgeB].position;
  const glm::vec3& p3 = clothData.particles[oppositeB].position;

  const glm::vec3 n0 = glm::cross(p1 - p0, p2 - p0);
  const glm::vec3 n1 = glm::cross(p2 - p3, p1 - p3);

  const float len0 = glm::length(n0);
  const float len1 = glm::length(n1);
  if (len0 <= 1e-6f || len1 <= 1e-6f) {
    return 0.0f;
  }

  const float dot = glm::clamp(glm::dot(n0 / len0, n1 / len1), -1.0f, 1.0f);
  return std::acos(dot);
}

} // namespace

size_t ClothData::pinnedParticleCount() const {
  return std::count_if(
      particles.begin(),
      particles.end(),
      [](const ClothParticle& particle) { return particle.pinned; });
}

size_t ClothData::staticParticleCount() const {
  return std::count_if(
      particles.begin(),
      particles.end(),
      [](const ClothParticle& particle) { return particle.isStatic(); });
}

std::optional<ClothData> buildClothDataFromMesh(
    const sauce::modeling::Mesh& mesh,
    const std::string& debugName,
    float defaultInvMass) {
  const auto& vertices = mesh.getVertices();
  const auto& indices = mesh.getIndices();

  if (vertices.empty() || indices.empty() || indices.size() % 3 != 0) {
    return std::nullopt;
  }

  ClothData clothData;
  clothData.debugName = debugName;
  clothData.particles.reserve(vertices.size());
  clothData.topology.particleIndices.reserve(vertices.size());
  clothData.topology.triangleIndices = indices;

  const float invMass = std::max(defaultInvMass, 0.0f);
  for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()); ++i) {
    const glm::vec3& position = vertices[i].position;
    clothData.particles.push_back({
        .position = position,
        .previousPosition = position,
        .predictedPosition = position,
        .velocity = glm::vec3(0.0f),
        .invMass = invMass,
        .pinned = false,
    });
    clothData.topology.particleIndices.push_back(i);
  }

  std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> edgeMap;
  edgeMap.reserve(indices.size());

  const size_t triangleCount = indices.size() / 3;
  for (size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex) {
    const size_t base = triangleIndex * 3;
    const std::array<uint32_t, 3> triangle {
        indices[base + 0],
        indices[base + 1],
        indices[base + 2],
    };

    for (uint32_t vertexIndex : triangle) {
      if (vertexIndex >= clothData.particles.size()) {
        return std::nullopt;
      }
    }

    const std::array<std::array<uint32_t, 2>, 3> triangleEdges {{
        { triangle[0], triangle[1] },
        { triangle[1], triangle[2] },
        { triangle[2], triangle[0] },
    }};

    for (const auto& triangleEdge : triangleEdges) {
      const EdgeKey edgeKey = makeEdgeKey(triangleEdge[0], triangleEdge[1]);
      auto edgeIt = edgeMap.find(edgeKey);

      if (edgeIt == edgeMap.end()) {
        const uint32_t edgeIndex = static_cast<uint32_t>(clothData.topology.edges.size());

        ClothEdge edge;
        edge.particleIndices = { edgeKey.a, edgeKey.b };
        edge.adjacentTriangleIndices[0] = static_cast<uint32_t>(triangleIndex);
        clothData.topology.edges.push_back(edge);
        edgeMap.emplace(edgeKey, edgeIndex);
        continue;
      }

      ClothEdge& edge = clothData.topology.edges[edgeIt->second];
      if (edge.adjacentTriangleIndices[0] == kInvalidClothIndex) {
        edge.adjacentTriangleIndices[0] = static_cast<uint32_t>(triangleIndex);
      } else if (edge.adjacentTriangleIndices[1] == kInvalidClothIndex) {
        edge.adjacentTriangleIndices[1] = static_cast<uint32_t>(triangleIndex);
      }
    }
  }

  clothData.stretchConstraints.reserve(clothData.topology.edges.size());
  for (const ClothEdge& edge : clothData.topology.edges) {
    const glm::vec3& p0 = clothData.particles[edge.particleIndices[0]].position;
    const glm::vec3& p1 = clothData.particles[edge.particleIndices[1]].position;

    clothData.stretchConstraints.emplace_back(
        edge.particleIndices[0],
        edge.particleIndices[1],
        glm::length(p1 - p0));
  }

  clothData.bendConstraints.reserve(clothData.topology.edges.size());
  for (uint32_t edgeIndex = 0;
       edgeIndex < static_cast<uint32_t>(clothData.topology.edges.size());
       ++edgeIndex) {
    const ClothEdge& edge = clothData.topology.edges[edgeIndex];
    if (edge.isBoundary()) {
      continue;
    }

    const auto triangleA = getTriangle(
        clothData.topology.triangleIndices,
        edge.adjacentTriangleIndices[0]);
    const auto triangleB = getTriangle(
        clothData.topology.triangleIndices,
        edge.adjacentTriangleIndices[1]);

    const uint32_t oppositeA = findOppositeParticle(
        triangleA,
        edge.particleIndices[0],
        edge.particleIndices[1]);
    const uint32_t oppositeB = findOppositeParticle(
        triangleB,
        edge.particleIndices[0],
        edge.particleIndices[1]);

    if (oppositeA == kInvalidClothIndex || oppositeB == kInvalidClothIndex) {
      continue;
    }

    clothData.bendConstraints.emplace_back(
        edgeIndex,
        edge.particleIndices[0],
        edge.particleIndices[1],
        oppositeA,
        oppositeB,
        computeRestBendAngle(
            clothData,
            edge.particleIndices[0],
            edge.particleIndices[1],
            oppositeA,
            oppositeB),
        edge.adjacentTriangleIndices[0],
        edge.adjacentTriangleIndices[1]);
  }

  return clothData;
}

} // namespace physics
