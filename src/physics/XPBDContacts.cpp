#include <physics/XPBD.hpp>
#include <physics/SphereBVH.hpp>
#include <physics/SphereCollider.hpp>
#include <physics/constraints/CollisionConstraint.hpp>
#include "RigidSolverShared.hpp"

#include <app/Entity.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <optional>
#include <numeric>
#include <tuple>
#include <unordered_map>

namespace physics {

namespace {

constexpr float kContactDepthEpsilon = 1e-5f;

struct CollisionBody;

struct BoxShape {
  glm::vec3 localCenter = glm::vec3(0.0f);
  glm::vec3 localHalfExtents = glm::vec3(0.0f);
};

struct PlaneShape {
  glm::vec3 localPoint = glm::vec3(0.0f);
  glm::vec3 localNormal = glm::vec3(0.0f, 0.0f, 1.0f);
};

struct MeshContact {
  const CollisionBody* bodyA = nullptr;
  const CollisionBody* bodyB = nullptr;
  glm::vec3 pointA = glm::vec3(0.0f);
  glm::vec3 pointB = glm::vec3(0.0f);
  glm::vec3 contactPoint = glm::vec3(0.0f);
  glm::vec3 contactNormal = glm::vec3(0.0f, 0.0f, 1.0f);
  float depth = 0.0f;
};

struct CollisionBody {
  uint32_t index = 0;
  sauce::RigidBodyComponent* rigidBody = nullptr;
  const sauce::modeling::Mesh* mesh = nullptr;
  const SphereBVHNode* bvhRoot = nullptr;
  std::vector<glm::vec3> worldVertices;
  std::vector<glm::vec3> sampleWorldVertices;
  float broadPhasePadding = 0.0f;
  float contactMargin = 0.0f;
  bool isBox = false;
  glm::vec3 boxCenter = glm::vec3(0.0f);
  glm::vec3 boxHalfExtents = glm::vec3(0.0f);
  bool isPlane = false;
  glm::vec3 planePoint = glm::vec3(0.0f);
  glm::vec3 planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);

  glm::vec3 axis(int axisIndex) const;
  float halfExtent(int axisIndex) const { return boxHalfExtents[axisIndex]; }
  std::array<glm::vec3, 8> corners() const;
  std::array<glm::vec3, 4> faceCorners(int normalAxisIndex, const glm::vec3& outwardNormal) const;
  SphereCollider broadPhaseSphere() const;
};

class MeshCollisionCache {
public:
  static MeshCollisionCache& instance() {
    static MeshCollisionCache cache;
    return cache;
  }

  const SphereBVHNode* getBVHRoot(const sauce::modeling::Mesh* mesh) {
    if (!mesh) {
      return nullptr;
    }

    auto it = bvhRoots.find(mesh);
    if (it == bvhRoots.end()) {
      auto built = SphereBVHNode::fromMesh(*const_cast<sauce::modeling::Mesh*>(mesh));
      it = bvhRoots.emplace(mesh, std::move(built)).first;
    }

    return it->second.get();
  }

  std::optional<BoxShape> getBoxShape(const sauce::modeling::Mesh* mesh) {
    if (!mesh) {
      return std::nullopt;
    }

    auto cached = boxShapes.find(mesh);
    if (cached != boxShapes.end()) {
      return cached->second;
    }

    const auto& vertices = mesh->getVertices();
    const auto& indices = mesh->getIndices();
    if (vertices.empty() || indices.empty()) {
      return boxShapes.emplace(mesh, std::nullopt).first->second;
    }

    glm::vec3 minExt(std::numeric_limits<float>::infinity());
    glm::vec3 maxExt(-std::numeric_limits<float>::infinity());
    for (const auto& vertex : vertices) {
      minExt = glm::min(minExt, vertex.position);
      maxExt = glm::max(maxExt, vertex.position);
    }

    const glm::vec3 center = 0.5f * (minExt + maxExt);
    const glm::vec3 halfExtents = 0.5f * (maxExt - minExt);
    const float tolerance = 1e-3f;

    if (glm::compMin(halfExtents) <= tolerance) {
      return boxShapes.emplace(mesh, std::nullopt).first->second;
    }

    std::array<bool, 8> cornerSeen = {false, false, false, false, false, false, false, false};
    for (const auto& vertex : vertices) {
      const glm::vec3 local = vertex.position - center;
      int cornerMask = 0;
      for (int axis = 0; axis < 3; ++axis) {
        const float value = local[axis];
        const float extent = halfExtents[axis];
        if (std::abs(std::abs(value) - extent) > tolerance) {
          return boxShapes.emplace(mesh, std::nullopt).first->second;
        }
        if (value > 0.0f) {
          cornerMask |= (1 << axis);
        }
      }
      cornerSeen[cornerMask] = true;
    }

    const bool allCornersPresent = std::all_of(cornerSeen.begin(), cornerSeen.end(), [](bool present) {
      return present;
    });
    if (!allCornersPresent) {
      return boxShapes.emplace(mesh, std::nullopt).first->second;
    }

    return boxShapes.emplace(mesh, BoxShape{center, halfExtents}).first->second;
  }

  std::optional<PlaneShape> getPlaneShape(const sauce::modeling::Mesh* mesh) {
    if (!mesh) {
      return std::nullopt;
    }

    auto cached = planeShapes.find(mesh);
    if (cached != planeShapes.end()) {
      return cached->second;
    }

    const auto& vertices = mesh->getVertices();
    const auto& indices = mesh->getIndices();
    if (vertices.size() < 3 || indices.size() < 3) {
      return planeShapes.emplace(mesh, std::nullopt).first->second;
    }

    const glm::vec3 p0 = vertices[indices[0]].position;
    const glm::vec3 p1 = vertices[indices[1]].position;
    const glm::vec3 p2 = vertices[indices[2]].position;
    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
    const float normalLength = glm::length(normal);
    if (normalLength <= 1e-8f) {
      return planeShapes.emplace(mesh, std::nullopt).first->second;
    }
    normal /= normalLength;

    constexpr float kPlaneTolerance = 1e-3f;
    for (const auto& vertex : vertices) {
      const float distance = glm::dot(vertex.position - p0, normal);
      if (std::abs(distance) > kPlaneTolerance) {
        return planeShapes.emplace(mesh, std::nullopt).first->second;
      }
    }

    return planeShapes.emplace(mesh, PlaneShape{p0, normal}).first->second;
  }

  float getBroadPhasePadding(const sauce::modeling::Mesh* mesh) {
    if (!mesh) {
      return 0.02f;
    }

    auto it = contactRadii.find(mesh);
    if (it != contactRadii.end()) {
      return it->second;
    }

    const float averageEdgeLength = averageEdgeLengthFor(mesh, 0.02f);
    const float radius = std::clamp(averageEdgeLength * 0.16f, 0.006f, 0.06f);
    return contactRadii.emplace(mesh, radius).first->second;
  }

  float getContactMargin(const sauce::modeling::Mesh* mesh) const {
    if (!mesh) {
      return 0.008f;
    }

    const float averageEdgeLength = averageEdgeLengthFor(mesh, 0.02f);
    return std::clamp(averageEdgeLength * 0.06f, 0.004f, 0.02f);
  }

  const std::vector<uint32_t>& getSampleVertexIndices(const sauce::modeling::Mesh* mesh) {
    static const std::vector<uint32_t> emptyIndices;
    if (!mesh) {
      return emptyIndices;
    }

    auto it = sampleVertexIndices.find(mesh);
    if (it != sampleVertexIndices.end()) {
      return it->second;
    }

    const auto& vertices = mesh->getVertices();
    std::vector<uint32_t> indices;
    indices.reserve(vertices.size());

    const size_t kMaxSampleVertices =
        std::min<size_t>(1024, std::max<size_t>(384, vertices.size() / 2));
    if (vertices.size() <= kMaxSampleVertices) {
      indices.resize(vertices.size());
      std::iota(indices.begin(), indices.end(), 0u);
      return sampleVertexIndices.emplace(mesh, std::move(indices)).first->second;
    }

    std::vector<uint8_t> seen(vertices.size(), 0);
    auto addSampleIndex = [&](uint32_t index) {
      if (index >= vertices.size() || seen[index]) {
        return;
      }
      seen[index] = 1;
      indices.push_back(index);
    };

    std::array<uint32_t, 6> extrema = {0, 0, 0, 0, 0, 0};
    for (uint32_t i = 1; i < static_cast<uint32_t>(vertices.size()); ++i) {
      const auto& position = vertices[i].position;
      if (position.x < vertices[extrema[0]].position.x) extrema[0] = i;
      if (position.x > vertices[extrema[1]].position.x) extrema[1] = i;
      if (position.y < vertices[extrema[2]].position.y) extrema[2] = i;
      if (position.y > vertices[extrema[3]].position.y) extrema[3] = i;
      if (position.z < vertices[extrema[4]].position.z) extrema[4] = i;
      if (position.z > vertices[extrema[5]].position.z) extrema[5] = i;
    }

    for (uint32_t index : extrema) {
      addSampleIndex(index);
    }

    const size_t remainingSlots = kMaxSampleVertices > indices.size()
        ? (kMaxSampleVertices - indices.size())
        : 0;
    const size_t stride = remainingSlots > 0
        ? std::max<size_t>(1, vertices.size() / remainingSlots)
        : vertices.size();

    for (size_t i = 0; i < vertices.size() && indices.size() < kMaxSampleVertices; i += stride) {
      addSampleIndex(static_cast<uint32_t>(i));
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()) &&
                         indices.size() < kMaxSampleVertices; ++i) {
      addSampleIndex(i);
    }

    return sampleVertexIndices.emplace(mesh, std::move(indices)).first->second;
  }

private:
  float averageEdgeLengthFor(const sauce::modeling::Mesh* mesh, float fallback) const {
    if (!mesh) {
      return fallback;
    }

    const auto& vertices = mesh->getVertices();
    const auto& indices = mesh->getIndices();
    if (vertices.empty() || indices.empty()) {
      return fallback;
    }

    double edgeSum = 0.0;
    size_t edgeCount = 0;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
      const glm::vec3& p0 = vertices[indices[i + 0]].position;
      const glm::vec3& p1 = vertices[indices[i + 1]].position;
      const glm::vec3& p2 = vertices[indices[i + 2]].position;
      edgeSum += glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p0 - p2);
      edgeCount += 3;
    }

    return edgeCount > 0
        ? static_cast<float>(edgeSum / static_cast<double>(edgeCount))
        : fallback;
  }

  std::unordered_map<const sauce::modeling::Mesh*, std::unique_ptr<SphereBVHNode>> bvhRoots;
  std::unordered_map<const sauce::modeling::Mesh*, float> contactRadii;
  std::unordered_map<const sauce::modeling::Mesh*, std::optional<BoxShape>> boxShapes;
  std::unordered_map<const sauce::modeling::Mesh*, std::optional<PlaneShape>> planeShapes;
  std::unordered_map<const sauce::modeling::Mesh*, std::vector<uint32_t>> sampleVertexIndices;
};

glm::vec3 scalePoint(const glm::vec3& point, const glm::vec3& scale) {
  return point * scale;
}

float maxScaleComponent(const glm::vec3& scale) {
  return std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
}

float minScaleComponent(const glm::vec3& scale) {
  return std::min({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
}

glm::vec3 absScale(const glm::vec3& scale) {
  return glm::vec3(std::abs(scale.x), std::abs(scale.y), std::abs(scale.z));
}

SphereCollider transformSphere(const SphereCollider& localSphere, const sauce::RigidBodyComponent& body) {
  SphereCollider worldSphere;
  worldSphere.center = body.getPosition() + body.getOrientation() * scalePoint(localSphere.center, body.getScale());
  worldSphere.radius = localSphere.radius * maxScaleComponent(body.getScale());
  return worldSphere;
}

glm::vec3 CollisionBody::axis(int axisIndex) const {
  switch (axisIndex) {
    case 0: return glm::normalize(rigidBody->getOrientation() * glm::vec3(1.0f, 0.0f, 0.0f));
    case 1: return glm::normalize(rigidBody->getOrientation() * glm::vec3(0.0f, 1.0f, 0.0f));
    default: return glm::normalize(rigidBody->getOrientation() * glm::vec3(0.0f, 0.0f, 1.0f));
  }
}

std::array<glm::vec3, 8> CollisionBody::corners() const {
  std::array<glm::vec3, 8> result;
  const glm::vec3 ex = rigidBody->getOrientation() * glm::vec3(boxHalfExtents.x, 0.0f, 0.0f);
  const glm::vec3 ey = rigidBody->getOrientation() * glm::vec3(0.0f, boxHalfExtents.y, 0.0f);
  const glm::vec3 ez = rigidBody->getOrientation() * glm::vec3(0.0f, 0.0f, boxHalfExtents.z);

  size_t cornerIndex = 0;
  for (int sx : {-1, 1}) {
    for (int sy : {-1, 1}) {
      for (int sz : {-1, 1}) {
        result[cornerIndex++] =
            boxCenter + static_cast<float>(sx) * ex + static_cast<float>(sy) * ey +
            static_cast<float>(sz) * ez;
      }
    }
  }
  return result;
}

std::array<glm::vec3, 4> CollisionBody::faceCorners(
    int normalAxisIndex,
    const glm::vec3& outwardNormal) const {
  const int tangentAxis0 = (normalAxisIndex + 1) % 3;
  const int tangentAxis1 = (normalAxisIndex + 2) % 3;
  const glm::vec3 tangent0 = axis(tangentAxis0);
  const glm::vec3 tangent1 = axis(tangentAxis1);
  const float extent0 = halfExtent(tangentAxis0);
  const float extent1 = halfExtent(tangentAxis1);
  const glm::vec3 faceCenter = boxCenter + outwardNormal * halfExtent(normalAxisIndex);

  return {
      faceCenter + tangent0 * extent0 + tangent1 * extent1,
      faceCenter - tangent0 * extent0 + tangent1 * extent1,
      faceCenter - tangent0 * extent0 - tangent1 * extent1,
      faceCenter + tangent0 * extent0 - tangent1 * extent1,
  };
}

SphereCollider CollisionBody::broadPhaseSphere() const {
  SphereCollider broad = transformSphere(bvhRoot->sphere, *rigidBody);
  broad.radius += broadPhasePadding;
  return broad;
}

bool spheresOverlap(const SphereCollider& a, const SphereCollider& b) {
  const float radiusSum = a.radius + b.radius;
  return glm::length2(b.center - a.center) <= radiusSum * radiusSum;
}

glm::vec3 closestPointOnTriangle(const glm::vec3& p,
                                 const glm::vec3& a,
                                 const glm::vec3& b,
                                 const glm::vec3& c) {
  const glm::vec3 ab = b - a;
  const glm::vec3 ac = c - a;
  const glm::vec3 ap = p - a;

  const float d1 = glm::dot(ab, ap);
  const float d2 = glm::dot(ac, ap);
  if (d1 <= 0.0f && d2 <= 0.0f) {
    return a;
  }

  const glm::vec3 bp = p - b;
  const float d3 = glm::dot(ab, bp);
  const float d4 = glm::dot(ac, bp);
  if (d3 >= 0.0f && d4 <= d3) {
    return b;
  }

  const float vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
    const float v = d1 / (d1 - d3);
    return a + v * ab;
  }

  const glm::vec3 cp = p - c;
  const float d5 = glm::dot(ab, cp);
  const float d6 = glm::dot(ac, cp);
  if (d6 >= 0.0f && d5 <= d6) {
    return c;
  }

  const float vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
    const float w = d2 / (d2 - d6);
    return a + w * ac;
  }

  const float va = d3 * d6 - d5 * d4;
  if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
    const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return b + w * (c - b);
  }

  const float denom = 1.0f / (va + vb + vc);
  const float v = vb * denom;
  const float w = vc * denom;
  return a + ab * v + ac * w;
}

void gatherCandidateTriangles(const SphereBVHNode* node,
                              const sauce::RigidBodyComponent& body,
                              const SphereCollider& query,
                              std::vector<uint32_t>& triangleIndices) {
  if (!node) {
    return;
  }

  const SphereCollider worldNode = transformSphere(node->sphere, body);
  if (!spheresOverlap(worldNode, query)) {
    return;
  }

  if (node->isLeaf()) {
    triangleIndices.insert(triangleIndices.end(), node->triangleIndices.begin(), node->triangleIndices.end());
    return;
  }

  gatherCandidateTriangles(node->left.get(), body, query, triangleIndices);
  gatherCandidateTriangles(node->right.get(), body, query, triangleIndices);
}

struct BoxFaceSelection {
  bool referenceIsA = true;
  int axisIndex = 0;
  glm::vec3 referenceOutwardNormal = glm::vec3(0.0f, 0.0f, 1.0f);
  float penetrationDepth = 0.0f;
  bool edgeEdge = false;
};

std::optional<BoxFaceSelection> selectBoxContactAxisSATImpl(const CollisionBody& bodyA,
                                                            const CollisionBody& bodyB,
                                                            bool includeEdgeEdge) {
  constexpr float kSatEpsilon = 1e-6f;
  const float contactMargin = 0.5f * (bodyA.contactMargin + bodyB.contactMargin);

  glm::mat3 rotation(0.0f);
  glm::mat3 absRotation(0.0f);
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      rotation[i][j] = glm::dot(bodyA.axis(i), bodyB.axis(j));
      absRotation[i][j] = std::abs(rotation[i][j]) + kSatEpsilon;
    }
  }

  const glm::vec3 centerDeltaWorld = bodyB.boxCenter - bodyA.boxCenter;
  const glm::vec3 centerDelta(
      glm::dot(centerDeltaWorld, bodyA.axis(0)),
      glm::dot(centerDeltaWorld, bodyA.axis(1)),
      glm::dot(centerDeltaWorld, bodyA.axis(2)));

  float minOverlap = std::numeric_limits<float>::infinity();
  std::optional<BoxFaceSelection> bestAxis;

  for (int i = 0; i < 3; ++i) {
    const float radiusA = bodyA.halfExtent(i);
    const float radiusB =
        bodyB.halfExtent(0) * absRotation[i][0] +
        bodyB.halfExtent(1) * absRotation[i][1] +
        bodyB.halfExtent(2) * absRotation[i][2];
    const float overlap = radiusA + radiusB + contactMargin - std::abs(centerDelta[i]);
    if (overlap < 0.0f) {
      return std::nullopt;
    }
    if (overlap < minOverlap) {
      minOverlap = overlap;
      bestAxis = BoxFaceSelection{
          .referenceIsA = true,
          .axisIndex = i,
          .referenceOutwardNormal = centerDelta[i] >= 0.0f ? bodyA.axis(i) : -bodyA.axis(i),
          .penetrationDepth = overlap,
          .edgeEdge = false,
      };
    }
  }

  for (int j = 0; j < 3; ++j) {
    const float radiusA =
        bodyA.halfExtent(0) * absRotation[0][j] +
        bodyA.halfExtent(1) * absRotation[1][j] +
        bodyA.halfExtent(2) * absRotation[2][j];
    const float radiusB = bodyB.halfExtent(j);
    const float distance =
        centerDelta.x * rotation[0][j] +
        centerDelta.y * rotation[1][j] +
        centerDelta.z * rotation[2][j];
    const float overlap = radiusA + radiusB + contactMargin - std::abs(distance);
    if (overlap < 0.0f) {
      return std::nullopt;
    }
    if (overlap < minOverlap) {
      minOverlap = overlap;
      bestAxis = BoxFaceSelection{
          .referenceIsA = false,
          .axisIndex = j,
          .referenceOutwardNormal = distance >= 0.0f ? -bodyB.axis(j) : bodyB.axis(j),
          .penetrationDepth = overlap,
          .edgeEdge = false,
      };
    }
  }

  if (!includeEdgeEdge) {
    return bestAxis;
  }

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      const float radiusA =
          bodyA.halfExtent((i + 1) % 3) * absRotation[(i + 2) % 3][j] +
          bodyA.halfExtent((i + 2) % 3) * absRotation[(i + 1) % 3][j];
      const float radiusB =
          bodyB.halfExtent((j + 1) % 3) * absRotation[i][(j + 2) % 3] +
          bodyB.halfExtent((j + 2) % 3) * absRotation[i][(j + 1) % 3];

      if (radiusA + radiusB < kSatEpsilon) {
        continue;
      }

      const float distance = std::abs(
          centerDelta[(i + 2) % 3] * rotation[(i + 1) % 3][j] -
          centerDelta[(i + 1) % 3] * rotation[(i + 2) % 3][j]);
      const float overlap = radiusA + radiusB + contactMargin - distance;
      if (overlap < 0.0f) {
        return std::nullopt;
      }
      constexpr float kEdgeBias = 2.5e-3f;
      if (overlap + kEdgeBias < minOverlap) {
        minOverlap = overlap;
        bestAxis = BoxFaceSelection{
            .referenceIsA = true,
            .axisIndex = i,
            .referenceOutwardNormal = glm::vec3(0.0f),
            .penetrationDepth = overlap,
            .edgeEdge = true,
        };
      }
    }
  }

  return bestAxis;
}

// Face axes only (no edge-edge tests). Used when the full SAT picks an edge-edge axis so we still
// get a stable face normal instead of falling back to mesh sampling.
std::optional<BoxFaceSelection> selectBoxContactFaceAxisSAT(const CollisionBody& bodyA,
                                                            const CollisionBody& bodyB) {
  return selectBoxContactAxisSATImpl(bodyA, bodyB, false);
}

std::optional<BoxFaceSelection> selectBoxContactAxisSAT(const CollisionBody& bodyA,
                                                        const CollisionBody& bodyB) {
  return selectBoxContactAxisSATImpl(bodyA, bodyB, true);
}

std::vector<glm::vec3> clipPolygonAgainstPlane(const std::vector<glm::vec3>& polygon,
                                               const glm::vec3& planePoint,
                                               const glm::vec3& planeNormal) {
  if (polygon.empty()) {
    return {};
  }

  std::vector<glm::vec3> result;
  result.reserve(polygon.size() + 1);
  auto signedDistance = [&](const glm::vec3& point) {
    return glm::dot(point - planePoint, planeNormal);
  };

  glm::vec3 previousPoint = polygon.back();
  float previousDistance = signedDistance(previousPoint);
  for (const glm::vec3& currentPoint : polygon) {
    const float currentDistance = signedDistance(currentPoint);
    const bool previousInside = previousDistance <= 0.0f;
    const bool currentInside = currentDistance <= 0.0f;

    if (previousInside != currentInside) {
      const float t = previousDistance / (previousDistance - currentDistance);
      result.push_back(previousPoint + t * (currentPoint - previousPoint));
    }
    if (currentInside) {
      result.push_back(currentPoint);
    }

    previousPoint = currentPoint;
    previousDistance = currentDistance;
  }

  return result;
}

bool generateBoxBoxContacts(const CollisionBody& bodyA,
                            const CollisionBody& bodyB,
                            std::vector<MeshContact>& contacts) {
  if (!bodyA.isBox || !bodyB.isBox) {
    return false;
  }

  const auto contactAxis = selectBoxContactAxisSAT(bodyA, bodyB);
  if (!contactAxis) {
    return true;
  }

  std::optional<BoxFaceSelection> faceAxis = contactAxis;
  if (contactAxis->edgeEdge) {
    faceAxis = selectBoxContactFaceAxisSAT(bodyA, bodyB);
    if (!faceAxis) {
      return false;
    }
  }

  if (faceAxis->edgeEdge) {
    return false;
  }

  const bool referenceIsA = faceAxis->referenceIsA;
  const CollisionBody& referenceBody = referenceIsA ? bodyA : bodyB;
  const CollisionBody& incidentBody = referenceIsA ? bodyB : bodyA;
  const glm::vec3 referenceOutwardNormal = faceAxis->referenceOutwardNormal;
  const glm::vec3 contactNormal = referenceIsA ? -referenceOutwardNormal : referenceOutwardNormal;
  const glm::vec3 referenceFaceCenter =
      referenceBody.boxCenter + referenceOutwardNormal * referenceBody.halfExtent(faceAxis->axisIndex);

  const glm::vec3 incidentSearchNormal = -referenceOutwardNormal;
  int incidentAxisIndex = 0;
  float incidentAlignment = -std::numeric_limits<float>::infinity();
  glm::vec3 incidentFaceNormal = incidentBody.axis(0);
  for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
    const glm::vec3 axis = incidentBody.axis(axisIndex);
    const float alignment = std::abs(glm::dot(axis, incidentSearchNormal));
    if (alignment <= incidentAlignment) {
      continue;
    }

    incidentAlignment = alignment;
    incidentAxisIndex = axisIndex;
    incidentFaceNormal = glm::dot(axis, incidentSearchNormal) >= 0.0f ? axis : -axis;
  }

  const auto incidentFaceCorners =
      incidentBody.faceCorners(incidentAxisIndex, incidentFaceNormal);
  std::vector<glm::vec3> clippedPolygon(incidentFaceCorners.begin(), incidentFaceCorners.end());

  const int tangentAxis0 = (faceAxis->axisIndex + 1) % 3;
  const int tangentAxis1 = (faceAxis->axisIndex + 2) % 3;
  const std::array<std::pair<glm::vec3, float>, 2> tangents = {{
      {referenceBody.axis(tangentAxis0), referenceBody.halfExtent(tangentAxis0)},
      {referenceBody.axis(tangentAxis1), referenceBody.halfExtent(tangentAxis1)},
  }};
  for (const auto& [tangent, extent] : tangents) {
    clippedPolygon = clipPolygonAgainstPlane(clippedPolygon, referenceFaceCenter + tangent * extent, tangent);
    clippedPolygon = clipPolygonAgainstPlane(clippedPolygon, referenceFaceCenter - tangent * extent, -tangent);
  }

  const float combinedMargin = bodyA.contactMargin + bodyB.contactMargin;
  for (const glm::vec3& pointOnIncident : clippedPolygon) {
    const float depth = glm::dot(pointOnIncident - referenceFaceCenter, contactNormal);
    if (depth < -combinedMargin) {
      continue;
    }

    const glm::vec3 pointOnReference = pointOnIncident - contactNormal * depth;
    if (referenceIsA) {
      contacts.push_back({
          &bodyA,
          &bodyB,
          pointOnReference,
          pointOnIncident,
          0.5f * (pointOnReference + pointOnIncident),
          contactNormal,
          depth,
      });
    } else {
      contacts.push_back({
          &bodyA,
          &bodyB,
          pointOnIncident,
          pointOnReference,
          0.5f * (pointOnReference + pointOnIncident),
          contactNormal,
          depth,
      });
    }
  }

  if (contacts.size() > 4) {
    std::sort(contacts.begin(), contacts.end(), [](const MeshContact& a, const MeshContact& b) {
      return a.depth > b.depth;
    });
    contacts.resize(4);
  }

  return true;
}

void generateBoxPlaneContacts(const CollisionBody& sampleBody,
                              const CollisionBody& targetBody,
                              std::vector<MeshContact>& contacts) {
  if (!sampleBody.isBox || !targetBody.isPlane) {
    return;
  }

  const auto sampleCorners = sampleBody.corners();
  for (const glm::vec3& worldCorner : sampleCorners) {
    const float signedDistance = glm::dot(worldCorner - targetBody.planePoint, targetBody.planeNormal);
    if (signedDistance > sampleBody.contactMargin) {
      continue;
    }

    const float depth = std::max(0.0f, -signedDistance);
    if (depth <= 0.0f) {
      continue;
    }

    const glm::vec3 worldPointOnPlane = worldCorner - signedDistance * targetBody.planeNormal;
    contacts.push_back({
        &sampleBody,
        &targetBody,
        worldCorner,
        worldPointOnPlane,
        0.5f * (worldCorner + worldPointOnPlane),
        targetBody.planeNormal,
        depth,
    });
  }

  if (contacts.size() > 4) {
    std::sort(contacts.begin(), contacts.end(), [](const MeshContact& a, const MeshContact& b) {
      return a.depth > b.depth;
    });
    contacts.resize(4);
  }
}

void generateVertexContacts(const CollisionBody& sampleBody,
                            const CollisionBody& targetBody,
                            std::vector<MeshContact>& contacts) {
  if (!sampleBody.bvhRoot || !targetBody.bvhRoot || !sampleBody.mesh || !targetBody.mesh) {
    return;
  }

  const auto& targetIndices = targetBody.mesh->getIndices();
  const glm::vec3 targetCenter = targetBody.rigidBody->getWorldCenterOfMass();

  for (const glm::vec3& worldVertex : sampleBody.sampleWorldVertices) {
    SphereCollider query;
    query.center = worldVertex;
    const float combinedMargin = sampleBody.contactMargin + targetBody.contactMargin;
    query.radius = combinedMargin;

    std::vector<uint32_t> candidateTriangles;
    gatherCandidateTriangles(targetBody.bvhRoot, *targetBody.rigidBody, query, candidateTriangles);
    if (candidateTriangles.empty()) {
      continue;
    }

    std::sort(candidateTriangles.begin(), candidateTriangles.end());
    candidateTriangles.erase(std::unique(candidateTriangles.begin(), candidateTriangles.end()),
                             candidateTriangles.end());

    float bestDistanceSq = std::numeric_limits<float>::infinity();
    glm::vec3 bestPoint(0.0f);
    glm::vec3 bestNormal(0.0f, 0.0f, 1.0f);
    float bestPenetrationDepth = 0.0f;

    for (uint32_t triangleIndex : candidateTriangles) {
      const size_t base = static_cast<size_t>(triangleIndex) * 3;
      if (base + 2 >= targetIndices.size()) {
        continue;
      }

      const glm::vec3& t0 = targetBody.worldVertices[targetIndices[base + 0]];
      const glm::vec3& t1 = targetBody.worldVertices[targetIndices[base + 1]];
      const glm::vec3& t2 = targetBody.worldVertices[targetIndices[base + 2]];
      const glm::vec3 closestPoint = closestPointOnTriangle(worldVertex, t0, t1, t2);
      const glm::vec3 delta = worldVertex - closestPoint;
      const float distanceSq = glm::dot(delta, delta);
      if (distanceSq > combinedMargin * combinedMargin ||
          distanceSq >= bestDistanceSq) {
        continue;
      }

      glm::vec3 normal;
      const glm::vec3 geometricNormalUnnormalized = glm::cross(t1 - t0, t2 - t0);
      const float geometricNormalLength = glm::length(geometricNormalUnnormalized);
      if (geometricNormalLength <= 1e-8f) {
        continue;
      }

      glm::vec3 geometricNormal = geometricNormalUnnormalized / geometricNormalLength;
      if (glm::dot(sampleBody.rigidBody->getWorldCenterOfMass() - targetCenter,
                   geometricNormal) < 0.0f) {
        geometricNormal = -geometricNormal;
      }

      if (distanceSq > 1e-10f) {
        normal = delta / std::sqrt(distanceSq);
        if (glm::dot(normal, geometricNormal) < 0.0f) {
          normal = -normal;
        }
      } else {
        normal = geometricNormal;
      }

      bestDistanceSq = distanceSq;
      bestPoint = closestPoint;
      bestNormal = normal;
      bestPenetrationDepth = std::max(0.0f, -glm::dot(worldVertex - closestPoint, geometricNormal));
    }

    if (!std::isfinite(bestDistanceSq)) {
      continue;
    }

    contacts.push_back({
      &sampleBody,
      &targetBody,
      worldVertex,
      bestPoint,
      0.5f * (worldVertex + bestPoint),
      bestNormal,
      bestPenetrationDepth,
    });
  }
}

void reduceContacts(std::vector<MeshContact>& contacts, float minPenetrationDepth) {
  constexpr size_t kMaxContacts = 8;
  constexpr float kMinContactSpacingSq = 0.0004f;

  contacts.erase(
      std::remove_if(contacts.begin(), contacts.end(), [minPenetrationDepth](const MeshContact& contact) {
        return contact.depth <= minPenetrationDepth;
      }),
      contacts.end());

  if (contacts.size() <= kMaxContacts) {
    return;
  }

  std::sort(contacts.begin(), contacts.end(), [](const MeshContact& a, const MeshContact& b) {
    return a.depth > b.depth;
  });

  std::vector<MeshContact> reduced;
  reduced.reserve(kMaxContacts);
  for (const auto& contact : contacts) {
    bool tooClose = false;
    for (const auto& kept : reduced) {
      if (glm::length2(contact.contactPoint - kept.contactPoint) < kMinContactSpacingSq) {
        tooClose = true;
        break;
      }
    }
    if (!tooClose) {
      reduced.push_back(contact);
    }
    if (reduced.size() == kMaxContacts) {
      break;
    }
  }

  if (reduced.empty()) {
    reduced.push_back(contacts.front());
  }
  contacts = std::move(reduced);
}

std::optional<CollisionBody> buildCollisionBody(uint32_t index,
                                                sauce::RigidBodyComponent* rigidBody) {
  if (!rigidBody || !rigidBody->isCollisionEnabled()) {
    return std::nullopt;
  }

  auto* owner = rigidBody->getOwner();
  if (!owner) {
    return std::nullopt;
  }

  auto* meshComp = owner->getComponent<sauce::MeshRendererComponent>();
  if (!meshComp || !meshComp->getMesh()) {
    return std::nullopt;
  }

  const auto& mesh = meshComp->getMesh();
  const auto& vertices = mesh->getVertices();
  if (vertices.empty()) {
    return std::nullopt;
  }

  const glm::vec3 bodyScale = rigidBody->getScale();
  std::vector<glm::vec3> worldVertices;
  worldVertices.reserve(vertices.size());
  for (const auto& vertex : vertices) {
    worldVertices.push_back(
        rigidBody->getPosition() + rigidBody->getOrientation() * scalePoint(vertex.position, bodyScale));
  }

  auto& cache = MeshCollisionCache::instance();
  const auto& sampleVertexIndices = cache.getSampleVertexIndices(mesh.get());
  std::vector<glm::vec3> sampleWorldVertices;
  sampleWorldVertices.reserve(sampleVertexIndices.size());
  for (uint32_t sampleIndex : sampleVertexIndices) {
    if (sampleIndex < worldVertices.size()) {
      sampleWorldVertices.push_back(worldVertices[sampleIndex]);
    }
  }
  if (sampleWorldVertices.empty()) {
    sampleWorldVertices = worldVertices;
  }

  const auto boxShape = cache.getBoxShape(mesh.get());
  const auto planeShape = cache.getPlaneShape(mesh.get());
  const glm::vec3 scaleAbs = absScale(bodyScale);
  const glm::vec3 worldBoxCenter = boxShape
      ? rigidBody->getPosition() + rigidBody->getOrientation() * scalePoint(boxShape->localCenter, bodyScale)
      : glm::vec3(0.0f);
  const glm::vec3 worldBoxHalfExtents = boxShape
      ? scalePoint(boxShape->localHalfExtents, scaleAbs)
      : glm::vec3(0.0f);
  const glm::vec3 worldPlanePoint = planeShape
      ? rigidBody->getPosition() + rigidBody->getOrientation() * scalePoint(planeShape->localPoint, bodyScale)
      : glm::vec3(0.0f);
  const glm::vec3 worldPlaneNormal = planeShape
      ? glm::normalize(rigidBody->getOrientation() * planeShape->localNormal)
      : glm::vec3(0.0f, 0.0f, 1.0f);

  return CollisionBody{
    index,
    rigidBody,
    mesh.get(),
    cache.getBVHRoot(mesh.get()),
    std::move(worldVertices),
    std::move(sampleWorldVertices),
    cache.getBroadPhasePadding(mesh.get()) * maxScaleComponent(bodyScale),
    cache.getContactMargin(mesh.get()) * minScaleComponent(bodyScale),
    boxShape.has_value(),
    worldBoxCenter,
    worldBoxHalfExtents,
    planeShape.has_value(),
    worldPlanePoint,
    worldPlaneNormal,
  };
}

struct CollisionDebugCounters {
  int noBvh = 0;
  int broadFail = 0;
  int narrowEmpty = 0;
  int depthFilter = 0;
  int accepted = 0;
  int boxSatSep = 0;
  int boxEdgeEdge = 0;
  int boxFaceEmpty = 0;
  int boxFaceOk = 0;

  void print(size_t bodyCount, bool enabled) const {
    if (!enabled) {
      return;
    }
    std::fprintf(stderr, "[collision] bodies=%zu noBvh=%d broadFail=%d narrowEmpty=%d depthFilter=%d accepted=%d satSep=%d edgeEdge=%d faceEmpty=%d faceOk=%d\n",
                 bodyCount, noBvh, broadFail, narrowEmpty, depthFilter, accepted,
                 boxSatSep, boxEdgeEdge, boxFaceEmpty, boxFaceOk);
  }
};

class PairContactBuilder {
public:
  PairContactBuilder(std::vector<MeshContact>& contacts, CollisionDebugCounters& debug)
      : contacts(contacts), debug(debug) {}

  void append(const CollisionBody& bodyA, const CollisionBody& bodyB) {
    if (bodyA.isBox && bodyB.isBox) {
      appendBoxPair(bodyA, bodyB);
      return;
    }
    appendDirection(bodyA, bodyB, contacts);
    appendDirection(bodyB, bodyA, contacts);
  }

private:
  static void appendGeneratedContacts(const CollisionBody& sampleBody,
                                      const CollisionBody& targetBody,
                                      float minPenetrationDepth,
                                      std::vector<MeshContact>& out,
                                      auto&& generate) {
    if (!sampleBody.rigidBody->canBeDynamic()) {
      return;
    }

    std::vector<MeshContact> generated;
    generate(sampleBody, targetBody, generated);
    reduceContacts(generated, minPenetrationDepth);
    out.insert(out.end(), generated.begin(), generated.end());
  }

  static void appendDirection(const CollisionBody& sampleBody,
                              const CollisionBody& targetBody,
                              std::vector<MeshContact>& out) {
    if (sampleBody.isBox && targetBody.isPlane) {
      appendGeneratedContacts(
          sampleBody, targetBody, kContactDepthEpsilon, out, generateBoxPlaneContacts);
      return;
    }
    appendGeneratedContacts(sampleBody, targetBody, 0.0005f, out, generateVertexContacts);
  }

  void appendBoxPair(const CollisionBody& bodyA, const CollisionBody& bodyB) {
    std::vector<MeshContact> boxContacts;
    const bool handledAnalytically = generateBoxBoxContacts(bodyA, bodyB, boxContacts);
    if (handledAnalytically) {
      boxContacts.empty() ? ++debug.boxSatSep : ++debug.boxFaceOk;
    } else {
      ++debug.boxEdgeEdge;
      appendGeneratedContacts(bodyA, bodyB, 0.0005f, boxContacts, generateVertexContacts);
      appendGeneratedContacts(bodyB, bodyA, 0.0005f, boxContacts, generateVertexContacts);
    }

    reduceContacts(boxContacts, -(bodyA.contactMargin + bodyB.contactMargin));
    if (boxContacts.empty() && handledAnalytically) {
      const auto axis = selectBoxContactAxisSAT(bodyA, bodyB);
      if (axis && !axis->edgeEdge && axis->penetrationDepth > 1e-5f) {
        ++debug.boxFaceEmpty;
      }
    }
    contacts.insert(contacts.end(), boxContacts.begin(), boxContacts.end());
  }

  std::vector<MeshContact>& contacts;
  CollisionDebugCounters& debug;
};

struct SupportInfo {
  sauce::RigidBodyComponent* rigidBody = nullptr;
  glm::vec3 center = glm::vec3(0.0f);
  float floorAltitude = 0.0f;
  float ceilingAltitude = 0.0f;
  float horizontalRadius = 0.0f;
  bool isPlane = false;
};

SupportInfo buildSupportInfo(const CollisionBody& body, const glm::vec3& up) {
  SupportInfo info;
  info.rigidBody = body.rigidBody;
  info.center = body.rigidBody->getWorldCenterOfMass();

  if (body.isPlane) {
    info.isPlane = true;
    info.floorAltitude = glm::dot(body.planePoint, up);
    info.ceilingAltitude = info.floorAltitude;
    info.horizontalRadius = std::numeric_limits<float>::max();
    return info;
  }

  if (body.isBox) {
    float verticalExtent = 0.0f;
    for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
      verticalExtent += std::abs(glm::dot(body.axis(axisIndex), up)) * body.halfExtent(axisIndex);
    }
    const float altitude = glm::dot(body.boxCenter, up);
    info.floorAltitude = altitude - verticalExtent;
    info.ceilingAltitude = altitude + verticalExtent;
    info.horizontalRadius = body.bvhRoot
        ? transformSphere(body.bvhRoot->sphere, *body.rigidBody).radius
        : glm::length(body.boxHalfExtents);
    return info;
  }

  if (body.bvhRoot) {
    const SphereCollider sphere = transformSphere(body.bvhRoot->sphere, *body.rigidBody);
    const float altitude = glm::dot(sphere.center, up);
    info.floorAltitude = altitude - sphere.radius;
    info.ceilingAltitude = altitude + sphere.radius;
    info.horizontalRadius = sphere.radius;
    return info;
  }

  const float altitude = glm::dot(info.center, up);
  info.floorAltitude = altitude;
  info.ceilingAltitude = altitude;
  info.horizontalRadius = 1.0f;
  return info;
}

} // namespace

std::vector<XPBDSolver::CollisionContact> XPBDSolver::collectCollisionContacts(
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies) const {
  std::vector<CollisionContact> contactsOut;

  std::vector<CollisionBody> bodies;
  bodies.reserve(rigidBodies.size());

  for (uint32_t i = 0; i < static_cast<uint32_t>(rigidBodies.size()); ++i) {
    if (auto body = buildCollisionBody(i, rigidBodies[i])) {
      bodies.push_back(std::move(*body));
    }
  }

  CollisionDebugCounters debug;
  for (size_t i = 0; i < bodies.size(); ++i) {
    for (size_t j = i + 1; j < bodies.size(); ++j) {
      if (!bodies[i].bvhRoot || !bodies[j].bvhRoot) {
        ++debug.noBvh;
        continue;
      }

      const SphereCollider broadA = bodies[i].broadPhaseSphere();
      const SphereCollider broadB = bodies[j].broadPhaseSphere();
      if (!spheresOverlap(broadA, broadB)) {
        ++debug.broadFail;
        continue;
      }

      std::vector<MeshContact> contacts;
      PairContactBuilder(contacts, debug).append(bodies[i], bodies[j]);

      if (contacts.empty()) {
        ++debug.narrowEmpty;
      }

      for (const auto& contact : contacts) {
        if (!contact.bodyA || !contact.bodyB) {
          ++debug.depthFilter;
          continue;
        }
        const float pairMargin = contact.bodyA->contactMargin + contact.bodyB->contactMargin;
        if (contact.depth < -pairMargin) {
          ++debug.depthFilter;
          continue;
        }

        ++debug.accepted;
        contactsOut.push_back({
            .indexA = contact.bodyA->index,
            .indexB = contact.bodyB->index,
            .pointA = contact.pointA,
            .pointB = contact.pointB,
            .contactNormal = contact.contactNormal,
            .penetrationDepth = contact.depth,
        });
      }
    }
  }

  debug.print(bodies.size(), contactDebugEnabled);

  return contactsOut;
}

std::vector<CollisionConstraint> XPBDSolver::generateCollisionConstraints(
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies) {
  auto contacts = collectCollisionContacts(rigidBodies);
  std::vector<bool> wasActiveBeforeWake(rigidBodies.size(), false);
  for (size_t i = 0; i < rigidBodies.size(); ++i) {
    auto* rb = rigidBodies[i];
    if (!rb) continue;
    wasActiveBeforeWake[i] = rb->isDynamic() ||
        (rb->canBeDynamic() && rb->getInvMass() <= 1e-8f && !rb->isSleeping());
  }

  struct ConstraintBuilder {
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies;
    const std::vector<bool>& wasActiveBeforeWake;
    const std::map<std::pair<uintptr_t, uintptr_t>, std::vector<float>>& collisionLambdaWarmStart;
    const sauce::RigidBodyComponent* dragBody;
    float dragContactFrictionScale;
    std::map<std::pair<uintptr_t, uintptr_t>, int> warmSlotForPair;
    std::vector<CollisionConstraint> constraints;

    ConstraintBuilder(const std::vector<sauce::RigidBodyComponent*>& rigidBodies,
                      const std::vector<bool>& wasActiveBeforeWake,
                      const std::map<std::pair<uintptr_t, uintptr_t>, std::vector<float>>& collisionLambdaWarmStart,
                      const sauce::RigidBodyComponent* dragBody,
                      float dragContactFrictionScale,
                      size_t reserveCount)
        : rigidBodies(rigidBodies),
          wasActiveBeforeWake(wasActiveBeforeWake),
          collisionLambdaWarmStart(collisionLambdaWarmStart),
          dragBody(dragBody),
          dragContactFrictionScale(dragContactFrictionScale) {
      constraints.reserve(reserveCount);
    }

    static bool lessThan(const CollisionContact& a,
                         const CollisionContact& b,
                         const std::vector<sauce::RigidBodyComponent*>& rigidBodies) {
      auto* aA = rigidBodies[a.indexA];
      auto* aB = rigidBodies[a.indexB];
      auto* bA = rigidBodies[b.indexA];
      auto* bB = rigidBodies[b.indexB];
      if (!aA || !aB) {
        return false;
      }
      if (!bA || !bB) {
        return true;
      }
      const auto ka = detail::canonicalRigidPair(aA, aB);
      const auto kb = detail::canonicalRigidPair(bA, bB);
      if (ka != kb) {
        return ka < kb;
      }
      return detail::quantizedWorldMid(0.5f * (a.pointA + a.pointB)) <
             detail::quantizedWorldMid(0.5f * (b.pointA + b.pointB));
    }

    bool shouldWake(uint32_t wakerIdx, sauce::RigidBodyComponent* sleeper) const {
      if (!sleeper->isSleeping() || !sleeper->canBeDynamic() || !wasActiveBeforeWake[wakerIdx]) {
        return false;
      }
      auto* waker = rigidBodies[wakerIdx];
      if (!waker) {
        return false;
      }
      constexpr float kKinematicWakeSpeedSq = 0.1f;
      return waker->isDynamic() || glm::length2(waker->getVelocity()) > kKinematicWakeSpeedSq;
    }

    float warmLambdaFor(sauce::RigidBodyComponent* bodyA, sauce::RigidBodyComponent* bodyB) {
      const auto pairKey = detail::canonicalRigidPair(bodyA, bodyB);
      const int slot = warmSlotForPair[pairKey]++;
      const auto warmIt = collisionLambdaWarmStart.find(pairKey);
      if (warmIt == collisionLambdaWarmStart.end() || slot >= static_cast<int>(warmIt->second.size())) {
        return 0.0f;
      }
      return warmIt->second[static_cast<size_t>(slot)];
    }

    float frictionFor(sauce::RigidBodyComponent* bodyA, sauce::RigidBodyComponent* bodyB) const {
      float frictionCoefficient = kRigidContactStaticFrictionMu;
      if (dragBody && (bodyA == dragBody || bodyB == dragBody)) {
        frictionCoefficient *= std::clamp(dragContactFrictionScale, 0.0f, 1.0f);
      }
      return frictionCoefficient;
    }

    void append(const CollisionContact& contact) {
      if (contact.indexA >= rigidBodies.size() || contact.indexB >= rigidBodies.size()) {
        return;
      }

      auto* bodyA = rigidBodies[contact.indexA];
      auto* bodyB = rigidBodies[contact.indexB];
      if (!bodyA || !bodyB) {
        return;
      }

      if (shouldWake(contact.indexA, bodyB)) { bodyB->wake(); }
      if (shouldWake(contact.indexB, bodyA)) { bodyA->wake(); }

      const glm::vec3 centerA = bodyA->getWorldCenterOfMass();
      const glm::vec3 centerB = bodyB->getWorldCenterOfMass();
      const glm::vec3 localOffsetA = glm::inverse(bodyA->getOrientation()) * (contact.pointA - centerA);
      const glm::vec3 localOffsetB = glm::inverse(bodyB->getOrientation()) * (contact.pointB - centerB);
      const glm::vec3 warmMid = 0.5f * (contact.pointA + contact.pointB);

      constraints.emplace_back(
          contact.indexA,
          contact.indexB,
          contact.contactNormal,
          localOffsetA,
          localOffsetB,
          0.0f,
          warmMid,
          warmLambdaFor(bodyA, bodyB),
          frictionFor(bodyA, bodyB));
    }
  };

  std::sort(contacts.begin(), contacts.end(), [&](const CollisionContact& a, const CollisionContact& b) {
    return ConstraintBuilder::lessThan(a, b, rigidBodies);
  });

  ConstraintBuilder builder(
      rigidBodies,
      wasActiveBeforeWake,
      collisionLambdaWarmStart,
      dragBody,
      dragContactFrictionScale,
      contacts.size());
  for (const auto& contact : contacts) {
    builder.append(contact);
  }
  return builder.constraints;
}

void XPBDSolver::wakeUnsupportedBodies(
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies) const {
  constexpr float kSupportVerticalMargin = 0.12f;

  const glm::vec3 up = -gravityDirection;

  std::vector<SupportInfo> infos;
  infos.reserve(rigidBodies.size());

  for (uint32_t i = 0; i < static_cast<uint32_t>(rigidBodies.size()); ++i) {
    auto body = buildCollisionBody(i, rigidBodies[i]);
    if (body) {
      infos.push_back(buildSupportInfo(*body, up));
    }
  }

  bool anyWoken = true;
  while (anyWoken) {
    anyWoken = false;
    for (size_t i = 0; i < infos.size(); ++i) {
      auto* body = infos[i].rigidBody;
      if (!body->isSleeping() || !body->canBeDynamic()) continue;

      const float sleepingFloor = infos[i].floorAltitude;
      const float sleepingAlt = glm::dot(infos[i].center, up);
      bool hasSupport = false;

      for (size_t j = 0; j < infos.size(); ++j) {
        if (i == j) continue;

        auto* supporter = infos[j].rigidBody;
        if (supporter->canBeDynamic() && !supporter->isSleeping()) continue;

        const float supportAlt = glm::dot(infos[j].center, up);
        if (supportAlt >= sleepingAlt) continue;

        if (infos[j].ceilingAltitude < sleepingFloor - kSupportVerticalMargin) continue;

        if (infos[j].isPlane) {
          hasSupport = true;
          break;
        }

        const glm::vec3 delta = infos[j].center - infos[i].center;
        const glm::vec3 horizontalDelta = delta - glm::dot(delta, up) * up;
        const float horizontalDist = glm::length(horizontalDelta);

        if (horizontalDist < infos[i].horizontalRadius + infos[j].horizontalRadius) {
          hasSupport = true;
          break;
        }
      }

      if (!hasSupport) {
        body->wake();
        anyWoken = true;
      }
    }
  }
}

} // namespace physics
