#include <app/PhysicsDemoSetup.hpp>
#include <app/Scene.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>
#include <app/components/TransformComponent.hpp>
#include <physics/XPBD.hpp>
#include <physics/constraints/Constraint.hpp>
#include <physics/constraints/CollisionConstraint.hpp>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <sstream>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

using sauce::Entity;
using sauce::MeshRendererComponent;
using sauce::RigidBodyComponent;
using sauce::Scene;
using sauce::TransformComponent;

struct BoxShape {
  glm::vec3 localCenter = glm::vec3(0.0f);
  glm::vec3 localHalfExtents = glm::vec3(0.0f);
};

struct PlaneShape {
  glm::vec3 localPoint = glm::vec3(0.0f);
  glm::vec3 localNormal = glm::vec3(0.0f, 0.0f, 1.0f);
};

struct OBB {
  std::string name;
  glm::vec3 center = glm::vec3(0.0f);
  std::array<glm::vec3, 3> axes{
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(0.0f, 0.0f, 1.0f)};
  glm::vec3 halfExtents = glm::vec3(0.5f);
  RigidBodyComponent* rigidBody = nullptr;
};

struct Plane {
  std::string name;
  glm::vec3 point = glm::vec3(0.0f);
  glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
};

struct OverlapStats {
  int pairCount = 0;
  float maxDepth = 0.0f;
  std::string bodyA;
  std::string bodyB;
};

struct PlanePenetrationStats {
  int pairCount = 0;
  float maxDepth = 0.0f;
  std::string body;
  std::string plane;
};

struct ContactDumpEntry {
  std::string bodyA;
  std::string bodyB;
  glm::vec3 normal = glm::vec3(0.0f);
};

struct PiecewiseTransferBlock {
  std::string name;
  RigidBodyComponent* rigidBody = nullptr;
  glm::quat targetOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  glm::vec3 extractWaypoint = glm::vec3(0.0f);
  glm::vec3 liftWaypoint = glm::vec3(0.0f);
  glm::vec3 travelWaypoint = glm::vec3(0.0f);
  glm::vec3 targetPosition = glm::vec3(0.0f);
  glm::vec3 hoverWaypoint = glm::vec3(0.0f);
};

struct PiecewiseTransferProgress {
  size_t activeIndex = 0;
  int phase = 0;
  int phaseSteps = 0;
  int settleFrames = 0;
  int completedBlocks = 0;
  float peakSpeed = 0.0f;
  float peakOverlap = 0.0f;
  float peakPlanePenetration = 0.0f;
};

glm::vec3 absVec3(const glm::vec3& v) {
  return glm::vec3(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

glm::vec3 scalePoint(const glm::vec3& point, const glm::vec3& scale) {
  return point * scale;
}

std::optional<BoxShape> detectBoxShape(const sauce::modeling::Mesh& mesh) {
  const auto& vertices = mesh.getVertices();
  if (vertices.empty()) {
    return std::nullopt;
  }

  glm::vec3 minExt(std::numeric_limits<float>::infinity());
  glm::vec3 maxExt(-std::numeric_limits<float>::infinity());
  for (const auto& vertex : vertices) {
    minExt = glm::min(minExt, vertex.position);
    maxExt = glm::max(maxExt, vertex.position);
  }

  const glm::vec3 center = 0.5f * (minExt + maxExt);
  const glm::vec3 halfExtents = 0.5f * (maxExt - minExt);
  if (std::min({halfExtents.x, halfExtents.y, halfExtents.z}) <= 1e-4f) {
    return std::nullopt;
  }

  constexpr float kTolerance = 1e-3f;
  std::array<bool, 8> cornerSeen = {false, false, false, false, false, false, false, false};
  for (const auto& vertex : vertices) {
    const glm::vec3 local = vertex.position - center;
    int cornerMask = 0;
    for (int axis = 0; axis < 3; ++axis) {
      const float value = local[axis];
      const float extent = halfExtents[axis];
      if (std::abs(std::abs(value) - extent) > kTolerance) {
        return std::nullopt;
      }
      if (value > 0.0f) {
        cornerMask |= (1 << axis);
      }
    }
    cornerSeen[cornerMask] = true;
  }

  if (!std::all_of(cornerSeen.begin(), cornerSeen.end(), [](bool seen) { return seen; })) {
    return std::nullopt;
  }

  return BoxShape{center, halfExtents};
}

std::optional<PlaneShape> detectPlaneShape(const sauce::modeling::Mesh& mesh) {
  const auto& vertices = mesh.getVertices();
  const auto& indices = mesh.getIndices();
  if (vertices.size() < 3 || indices.size() < 3) {
    return std::nullopt;
  }

  const glm::vec3 p0 = vertices[indices[0]].position;
  const glm::vec3 p1 = vertices[indices[1]].position;
  const glm::vec3 p2 = vertices[indices[2]].position;
  glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
  const float len = glm::length(normal);
  if (len <= 1e-8f) {
    return std::nullopt;
  }
  normal /= len;

  constexpr float kTolerance = 1e-3f;
  for (const auto& vertex : vertices) {
    if (std::abs(glm::dot(vertex.position - p0, normal)) > kTolerance) {
      return std::nullopt;
    }
  }

  return PlaneShape{p0, normal};
}

std::optional<OBB> buildOBB(Entity& entity) {
  auto* rigidBody = entity.getComponent<RigidBodyComponent>();
  auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
  if (!rigidBody || !meshRenderer || !meshRenderer->getMesh()) {
    return std::nullopt;
  }

  const auto box = detectBoxShape(*meshRenderer->getMesh());
  if (!box) {
    return std::nullopt;
  }

  const glm::quat orientation = rigidBody->getOrientation();
  const glm::vec3 scale = rigidBody->getScale();
  const glm::vec3 scaleAbs = absVec3(scale);

  OBB obb;
  obb.name = entity.get_name();
  obb.center = rigidBody->getPosition() + orientation * scalePoint(box->localCenter, scale);
  obb.axes = {
      glm::normalize(orientation * glm::vec3(1.0f, 0.0f, 0.0f)),
      glm::normalize(orientation * glm::vec3(0.0f, 1.0f, 0.0f)),
      glm::normalize(orientation * glm::vec3(0.0f, 0.0f, 1.0f)),
  };
  obb.halfExtents = scalePoint(box->localHalfExtents, scaleAbs);
  obb.rigidBody = rigidBody;
  return obb;
}

std::optional<Plane> buildPlane(Entity& entity) {
  auto* rigidBody = entity.getComponent<RigidBodyComponent>();
  auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
  if (!rigidBody || !meshRenderer || !meshRenderer->getMesh()) {
    return std::nullopt;
  }

  const auto planeShape = detectPlaneShape(*meshRenderer->getMesh());
  if (!planeShape) {
    return std::nullopt;
  }

  Plane plane;
  plane.name = entity.get_name();
  plane.point = rigidBody->getPosition() +
      rigidBody->getOrientation() * scalePoint(planeShape->localPoint, rigidBody->getScale());
  plane.normal = glm::normalize(rigidBody->getOrientation() * planeShape->localNormal);
  return plane;
}

std::array<glm::vec3, 8> computeCorners(const OBB& obb) {
  std::array<glm::vec3, 8> corners;
  size_t index = 0;
  for (int sx : {-1, 1}) {
    for (int sy : {-1, 1}) {
      for (int sz : {-1, 1}) {
        corners[index++] = obb.center +
            static_cast<float>(sx) * obb.axes[0] * obb.halfExtents.x +
            static_cast<float>(sy) * obb.axes[1] * obb.halfExtents.y +
            static_cast<float>(sz) * obb.axes[2] * obb.halfExtents.z;
      }
    }
  }
  return corners;
}

std::optional<float> computeOBBOverlapDepth(const OBB& a, const OBB& b) {
  constexpr float kEpsilon = 1e-6f;

  glm::mat3 R(0.0f);
  glm::mat3 absR(0.0f);
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      R[i][j] = glm::dot(a.axes[i], b.axes[j]);
      absR[i][j] = std::abs(R[i][j]) + kEpsilon;
    }
  }

  const glm::vec3 tWorld = b.center - a.center;
  const glm::vec3 t(
      glm::dot(tWorld, a.axes[0]),
      glm::dot(tWorld, a.axes[1]),
      glm::dot(tWorld, a.axes[2]));

  float minOverlap = std::numeric_limits<float>::infinity();
  auto testAxis = [&](float distance, float radiusA, float radiusB) -> bool {
    const float overlap = radiusA + radiusB - std::abs(distance);
    if (overlap < 0.0f) {
      return false;
    }
    minOverlap = std::min(minOverlap, overlap);
    return true;
  };

  for (int i = 0; i < 3; ++i) {
    const float ra = a.halfExtents[i];
    const float rb = b.halfExtents.x * absR[i][0] + b.halfExtents.y * absR[i][1] + b.halfExtents.z * absR[i][2];
    if (!testAxis(t[i], ra, rb)) {
      return std::nullopt;
    }
  }

  for (int j = 0; j < 3; ++j) {
    const float ra = a.halfExtents.x * absR[0][j] + a.halfExtents.y * absR[1][j] + a.halfExtents.z * absR[2][j];
    const float rb = b.halfExtents[j];
    const float distance = t.x * R[0][j] + t.y * R[1][j] + t.z * R[2][j];
    if (!testAxis(distance, ra, rb)) {
      return std::nullopt;
    }
  }

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      const float ra = a.halfExtents[(i + 1) % 3] * absR[(i + 2) % 3][j] +
          a.halfExtents[(i + 2) % 3] * absR[(i + 1) % 3][j];
      const float rb = b.halfExtents[(j + 1) % 3] * absR[i][(j + 2) % 3] +
          b.halfExtents[(j + 2) % 3] * absR[i][(j + 1) % 3];
      const float distance = std::abs(
          t[(i + 2) % 3] * R[(i + 1) % 3][j] -
          t[(i + 1) % 3] * R[(i + 2) % 3][j]);
      if (!testAxis(distance, ra, rb)) {
        return std::nullopt;
      }
    }
  }

  if (!std::isfinite(minOverlap)) {
    return std::nullopt;
  }
  return minOverlap;
}

float computePlanePenetrationDepth(const OBB& box, const Plane& plane) {
  const auto corners = computeCorners(box);
  float maxDepth = 0.0f;
  for (const glm::vec3& corner : corners) {
    const float signedDistance = glm::dot(corner - plane.point, plane.normal);
    maxDepth = std::max(maxDepth, -signedDistance);
  }
  return std::max(0.0f, maxDepth);
}

size_t dynamicBodyCount(const std::vector<RigidBodyComponent*>& rigidBodies) {
  return static_cast<size_t>(std::count_if(
      rigidBodies.begin(),
      rigidBodies.end(),
      [](const RigidBodyComponent* rigidBody) {
        return rigidBody && rigidBody->canBeDynamic() && rigidBody->isCollisionEnabled();
      }));
}

void collectDiagnostics(
    Scene& scene,
    OverlapStats& overlapStats,
    PlanePenetrationStats& planeStats,
    float& maxSpeed) {
  std::vector<OBB> dynamicBoxes;
  std::vector<Plane> staticPlanes;
  maxSpeed = 0.0f;

  for (auto& entity : scene.getEntitiesMut()) {
    auto* rigidBody = entity.getComponent<RigidBodyComponent>();
    if (!rigidBody) {
      continue;
    }

    maxSpeed = std::max(maxSpeed, glm::length(rigidBody->getVelocity()));
    if (rigidBody->isDynamic()) {
      if (auto box = buildOBB(entity)) {
        dynamicBoxes.push_back(*box);
      }
    } else if (auto plane = buildPlane(entity)) {
      staticPlanes.push_back(*plane);
    }
  }

  overlapStats = {};
  for (size_t i = 0; i < dynamicBoxes.size(); ++i) {
    for (size_t j = i + 1; j < dynamicBoxes.size(); ++j) {
      const auto overlap = computeOBBOverlapDepth(dynamicBoxes[i], dynamicBoxes[j]);
      if (!overlap || *overlap <= 1e-5f) {
        continue;
      }
      ++overlapStats.pairCount;
      if (*overlap > overlapStats.maxDepth) {
        overlapStats.maxDepth = *overlap;
        overlapStats.bodyA = dynamicBoxes[i].name;
        overlapStats.bodyB = dynamicBoxes[j].name;
      }
    }
  }

  planeStats = {};
  for (const auto& box : dynamicBoxes) {
    for (const auto& plane : staticPlanes) {
      const float depth = computePlanePenetrationDepth(box, plane);
      if (depth <= 1e-5f) {
        continue;
      }
      ++planeStats.pairCount;
      if (depth > planeStats.maxDepth) {
        planeStats.maxDepth = depth;
        planeStats.body = box.name;
        planeStats.plane = plane.name;
      }
    }
  }
}

RigidBodyComponent* findRigidBodyByName(
    const std::vector<RigidBodyComponent*>& rigidBodies,
    std::string_view name) {
  for (auto* rigidBody : rigidBodies) {
    auto* owner = rigidBody ? rigidBody->getOwner() : nullptr;
    if (owner && owner->get_name() == name) {
      return rigidBody;
    }
  }
  return nullptr;
}

int parseJengaBlockIndex(std::string_view name) {
  constexpr std::string_view prefix = "JengaBlock_";
  if (!name.starts_with(prefix)) {
    return -1;
  }
  int value = 0;
  for (size_t i = prefix.size(); i < name.size(); ++i) {
    const char c = name[i];
    if (c < '0' || c > '9') {
      return -1;
    }
    value = value * 10 + (c - '0');
  }
  return value;
}

glm::vec3 clampVectorMagnitude(const glm::vec3& value, float maxMagnitude) {
  const float lengthSq = glm::length2(value);
  if (maxMagnitude <= 0.0f || lengthSq <= maxMagnitude * maxMagnitude) {
    return value;
  }
  return value * (maxMagnitude / std::sqrt(lengthSq));
}

glm::vec3 horizontalLongAxis(const glm::quat& orientation) {
  const glm::vec3 axis = orientation * glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::vec3 horizontal(axis.x, axis.y, 0.0f);
  if (glm::length2(horizontal) <= 1e-6f) {
    return glm::vec3(1.0f, 0.0f, 0.0f);
  }
  return glm::normalize(horizontal);
}

glm::vec3 piecewiseTransferTarget(const PiecewiseTransferBlock& block, int phase) {
  switch (phase) {
    case 0:
      return block.extractWaypoint;
    case 1:
      return block.liftWaypoint;
    case 2:
      return block.travelWaypoint;
    case 3:
      return block.hoverWaypoint;
    default:
      return block.hoverWaypoint;
  }
}

std::vector<PiecewiseTransferBlock> buildPiecewiseTransferPlan(
    const std::vector<RigidBodyComponent*>& rigidBodies) {
  std::unordered_map<int, glm::vec3> initialPositions;
  std::unordered_map<int, glm::quat> initialOrientations;
  initialPositions.reserve(rigidBodies.size());
  initialOrientations.reserve(rigidBodies.size());
  for (auto* rigidBody : rigidBodies) {
    auto* owner = rigidBody ? rigidBody->getOwner() : nullptr;
    if (!owner) {
      continue;
    }
    const int blockIndex = parseJengaBlockIndex(owner->get_name());
    if (blockIndex < 0) {
      continue;
    }
    initialPositions[blockIndex] = rigidBody->getPosition();
    initialOrientations[blockIndex] = rigidBody->getOrientation();
  }

  const glm::vec3 up(0.0f, 0.0f, 1.0f);
  const glm::vec3 towerOffset(6.0f, 0.0f, 0.0f);
  constexpr float kExtractDistance = 1.00f;
  constexpr float kExtractLift = 0.10f;
  constexpr float kCarryHeight = 5.85f;
  constexpr float kHoverHeight = 0.42f;

  std::vector<PiecewiseTransferBlock> plan;
  plan.reserve(9);
  constexpr std::array<int, 3> kSlotOrder = {0, 2, 1};

  for (int targetLayer = 0; targetLayer < 3; ++targetLayer) {
    const int sourceLayer = (targetLayer % 2 == 0) ? (6 - targetLayer) : (8 - targetLayer);
    for (int slot : kSlotOrder) {
      const int targetIndex = targetLayer * 3 + slot;
      const int sourceIndex = sourceLayer * 3 + slot;

      auto* rigidBody = findRigidBodyByName(
          rigidBodies, std::format("JengaBlock_{}", sourceIndex));
      const auto sourcePosIt = initialPositions.find(sourceIndex);
      const auto targetPosIt = initialPositions.find(targetIndex);
      const auto targetOriIt = initialOrientations.find(targetIndex);
      if (!rigidBody || sourcePosIt == initialPositions.end() ||
          targetPosIt == initialPositions.end() || targetOriIt == initialOrientations.end()) {
        continue;
      }

      const glm::vec3 sourcePosition = sourcePosIt->second;
      const glm::quat targetOrientation = targetOriIt->second;
      const glm::vec3 targetPosition = targetPosIt->second + towerOffset;
      const glm::vec3 longAxis = horizontalLongAxis(targetOrientation);

      float extractSign = 0.0f;
      const float radial = glm::dot(sourcePosition, longAxis);
      if (std::abs(radial) > 0.25f) {
        extractSign = radial > 0.0f ? 1.0f : -1.0f;
      } else {
        const float towardTarget = glm::dot(towerOffset, longAxis);
        if (std::abs(towardTarget) > 0.25f) {
          extractSign = towardTarget > 0.0f ? 1.0f : -1.0f;
        } else {
          extractSign = slot == 0 ? -1.0f : 1.0f;
        }
      }
      const glm::vec3 extractDir = longAxis * extractSign;
      const glm::vec3 extractWaypoint =
          sourcePosition + extractDir * kExtractDistance + up * kExtractLift;
      const glm::vec3 liftWaypoint(extractWaypoint.x, extractWaypoint.y, kCarryHeight);
      const glm::vec3 travelWaypoint(targetPosition.x, targetPosition.y, kCarryHeight);

      plan.push_back(PiecewiseTransferBlock{
          .name = std::format("JengaBlock_{}", sourceIndex),
          .rigidBody = rigidBody,
          .targetOrientation = targetOrientation,
          .extractWaypoint = extractWaypoint,
          .liftWaypoint = liftWaypoint,
          .travelWaypoint = travelWaypoint,
          .targetPosition = targetPosition,
          .hoverWaypoint = targetPosition + up * kHoverHeight,
      });
    }
  }

  return plan;
}

bool advancePiecewiseTransferIfReady(
    PiecewiseTransferProgress& progress,
    const std::vector<PiecewiseTransferBlock>& plan) {
  if (progress.activeIndex >= plan.size()) {
    return false;
  }

  const auto& block = plan[progress.activeIndex];
  auto* rigidBody = block.rigidBody;
  if (!rigidBody) {
    ++progress.activeIndex;
    progress.phase = 0;
    progress.phaseSteps = 0;
    progress.settleFrames = 0;
    progress.completedBlocks = static_cast<int>(progress.activeIndex);
    return progress.activeIndex < plan.size();
  }

  if (progress.phase < 4) {
    const glm::vec3 target = piecewiseTransferTarget(block, progress.phase);
    const float distance = glm::distance(rigidBody->getPosition(), target);
    const float speed = glm::length(rigidBody->getVelocity());
    const float phaseDistanceThreshold = progress.phase == 3 ? 0.18f : 0.12f;
    const float phaseSpeedThreshold = progress.phase == 3 ? 0.80f : 0.42f;
    const bool hoverTimeout = progress.phase == 3 && progress.phaseSteps > 240;

    const bool reached =
        hoverTimeout ||
        (distance <= phaseDistanceThreshold &&
         speed <= phaseSpeedThreshold);
    if (reached) {
      ++progress.phase;
      progress.phaseSteps = 0;
      if (progress.phase == 4) {
        rigidBody->setCollisionEnabled(true);
        rigidBody->wake();
        rigidBody->setVelocity(glm::vec3(0.0f, 0.0f, -0.42f));
        rigidBody->setAngularVelocity(glm::vec3(0.0f));
      }
    }
    return true;
  }

  const float distance = glm::distance(rigidBody->getPosition(), block.targetPosition);
  const float speed = glm::length(rigidBody->getVelocity());

  constexpr float kSettleDistanceThreshold = 0.10f;
  constexpr float kSettleSpeedThreshold = 0.14f;
  constexpr int kPlaceSettleFrames = 20;

  if ((distance <= kSettleDistanceThreshold && speed <= kSettleSpeedThreshold) ||
      rigidBody->isSleeping()) {
    ++progress.settleFrames;
  } else {
    progress.settleFrames = 0;
  }

  if (progress.settleFrames < kPlaceSettleFrames) {
    return true;
  }

  ++progress.activeIndex;
  progress.phase = 0;
  progress.phaseSteps = 0;
  progress.settleFrames = 0;
  progress.completedBlocks = static_cast<int>(progress.activeIndex);
  return progress.activeIndex < plan.size();
}

void drivePiecewiseTransferBlock(
    physics::XPBDSolver& solver,
    const PiecewiseTransferBlock& block,
    int phase,
    float physicsDt) {
  auto* rigidBody = block.rigidBody;
  if (!rigidBody) {
    solver.dragBody = nullptr;
    return;
  }

  solver.dragBody = rigidBody;
  rigidBody->wake();
  rigidBody->setCollisionEnabled(false);
  rigidBody->setOrientation(block.targetOrientation);
  const glm::vec3 gravityDir = solver.gravityDirection;
  const glm::vec3 target = piecewiseTransferTarget(block, phase);
  const glm::vec3 toTarget = target - rigidBody->getPosition();
  const float targetDistance = glm::length(toTarget);
  const float catchupBlend = glm::smoothstep(0.12f, 0.9f, targetDistance);
  const float phaseScale =
      phase == 0 ? 1.20f : (phase == 3 ? 0.72f : 0.90f);
  const glm::vec3 currentVelocity = rigidBody->getVelocity();
  const glm::vec3 desiredVelocity = toTarget * (6.4f * phaseScale);
  const float verticalSpeed = glm::dot(desiredVelocity, gravityDir);
  const glm::vec3 desiredVerticalVelocity = verticalSpeed * gravityDir;
  const glm::vec3 desiredLateralVelocity = desiredVelocity - desiredVerticalVelocity;

  const float lateralSpeedLimit = std::lerp(1.8f, 4.2f, catchupBlend) * phaseScale;
  const float downwardSpeedLimit = std::lerp(1.3f, 3.4f, catchupBlend) * phaseScale;
  const float upwardSpeedLimit = std::lerp(1.7f, 3.9f, catchupBlend) * phaseScale;
  const glm::vec3 clampedDesiredVelocity =
      clampVectorMagnitude(desiredLateralVelocity, lateralSpeedLimit) +
      clampVectorMagnitude(
          desiredVerticalVelocity,
          verticalSpeed > 0.0f ? downwardSpeedLimit : upwardSpeedLimit);

  const glm::vec3 desiredAcceleration =
      (clampedDesiredVelocity - currentVelocity) / std::max(physicsDt, 1e-4f);
  const float verticalAcceleration = glm::dot(desiredAcceleration, gravityDir);
  const glm::vec3 desiredVerticalAcceleration = verticalAcceleration * gravityDir;
  const glm::vec3 desiredLateralAcceleration =
      desiredAcceleration - desiredVerticalAcceleration;

  const float lateralAccelerationLimit = std::lerp(14.0f, 32.0f, catchupBlend) * phaseScale;
  const float downwardAccelerationLimit = std::lerp(11.0f, 24.0f, catchupBlend) * phaseScale;
  const float upwardAccelerationLimit = std::lerp(13.0f, 28.0f, catchupBlend) * phaseScale;
  const glm::vec3 driveVelocity =
      currentVelocity +
      (clampVectorMagnitude(desiredLateralAcceleration, lateralAccelerationLimit) +
       clampVectorMagnitude(
           desiredVerticalAcceleration,
           verticalAcceleration > 0.0f ? downwardAccelerationLimit : upwardAccelerationLimit)) *
          physicsDt;

  rigidBody->setVelocity(driveVelocity);
  rigidBody->setAngularVelocity(rigidBody->getAngularVelocity() * 0.10f);
}

std::vector<ContactDumpEntry> collectContactDump(
    physics::XPBDSolver& solver,
    const std::vector<RigidBodyComponent*>& rigidBodies) {
  auto constraints = solver.generateCollisionConstraints(rigidBodies);
  std::vector<ContactDumpEntry> dump;
  dump.reserve(constraints.size());

  for (const auto& constraint : constraints) {
    const auto* collision = dynamic_cast<const physics::CollisionConstraint*>(constraint.get());
    if (!collision) {
      continue;
    }

    if (collision->indexA >= rigidBodies.size() || collision->indexB >= rigidBodies.size()) {
      continue;
    }

    auto* bodyA = rigidBodies[collision->indexA];
    auto* bodyB = rigidBodies[collision->indexB];
    const auto* ownerA = bodyA ? bodyA->getOwner() : nullptr;
    const auto* ownerB = bodyB ? bodyB->getOwner() : nullptr;
    dump.push_back({
        ownerA ? ownerA->get_name() : "?",
        ownerB ? ownerB->get_name() : "?",
        collision->contactNormal,
    });
  }

  std::sort(dump.begin(), dump.end(), [](const ContactDumpEntry& a, const ContactDumpEntry& b) {
    return a.bodyA < b.bodyA;
  });
  return dump;
}

} // namespace

int main(int argc, char** argv) {
  const bool testStackStability =
      argc >= 2 && std::strcmp(argv[1], "--test-stack-stability") == 0;
  const bool testPiecewiseTransfer =
      argc >= 2 && std::strcmp(argv[1], "--test-piecewise-transfer") == 0;

  std::string scenePath = "testScene/jenga_tower.gltf";
  int steps = 180;
  int dumpStep = -1;
  int wakeStep = -1;
  std::string wakeBodyName = "JengaBlock_0";
  float wakeSpeed = 1.2f;
  std::string removeBodyName;
  int removeStep = -1;

  if (testStackStability) {
    scenePath = "testScene/jenga_tower.gltf";
    steps = 360;
    dumpStep = -1;
    wakeStep = -1;
    removeStep = -1;
    removeBodyName.clear();
    std::cout << "[--test-stack-stability] settled Jenga OBB overlap regression (no wake)\n";
  } else if (testPiecewiseTransfer) {
    scenePath = "testScene/jenga_tower.gltf";
    steps = 5200;
    dumpStep = -1;
    wakeStep = -1;
    removeStep = -1;
    removeBodyName.clear();
    std::cout << "[--test-piecewise-transfer] scripted drag rebuild of a second three-layer tower\n";
  } else {
    if (argc >= 2) {
      scenePath = argv[1];
    }
    if (argc >= 3) {
      steps = std::max(1, std::atoi(argv[2]));
    }
    if (argc >= 4) {
      dumpStep = std::atoi(argv[3]);
    }
    if (argc >= 5) {
      wakeStep = std::atoi(argv[4]);
    }
    if (argc >= 6) {
      wakeBodyName = argv[5];
    }
    if (argc >= 7) {
      wakeSpeed = static_cast<float>(std::atof(argv[6]));
    }
    if (argc >= 8) {
      removeBodyName = argv[7];
    }
    if (argc >= 9) {
      removeStep = std::atoi(argv[8]);
    }
  }
  const glm::vec3 wakeVelocity(wakeSpeed, 0.0f, 0.0f);

  float maxSettledBoxOverlap = 0.0f;
  constexpr int kStackTestSettleAfterStep = 200;
  constexpr float kStackTestMaxBoxOverlapM = 0.048f;
  constexpr float kTransferTestPeakSpeedLimit = 6.5f;
  constexpr float kTransferTestPeakOverlapLimitM = 0.02f;
  constexpr float kTransferTestPeakPlanePenetrationLimitM = 0.02f;
  constexpr float kTransferTestFinalPlacementToleranceM = 0.16f;

  sauce::CameraCreateInfo cameraInfo{
      .scrWidth = 1280.0f,
      .scrHeight = 720.0f,
  };

  Scene scene(cameraInfo);
  if (!scene.loadFromFile(scenePath)) {
    std::cerr << "Failed to load scene: " << scenePath << "\n";
    return 1;
  }

  sauce::physics_demo::armScene(scene);
  auto rigidBodies = sauce::physics_demo::collectRigidBodies(scene);

  physics::XPBDSolver solver;
  const auto tuning = sauce::physics_demo::selectRigidSolverTuning(dynamicBodyCount(rigidBodies));
  solver.solverIterations = tuning.solverIterations;
  solver.rigidSubsteps = tuning.rigidSubsteps;
  solver.contactDebugEnabled = false;
  if (testPiecewiseTransfer) {
    solver.dragContactFrictionScale = 0.005f;
  }

  auto transferPlan = std::vector<PiecewiseTransferBlock>();
  PiecewiseTransferProgress transferProgress;
  if (testPiecewiseTransfer) {
    transferPlan = buildPiecewiseTransferPlan(rigidBodies);
    if (transferPlan.size() != 9) {
      std::cerr << "FAIL: could not build scripted transfer plan from Jenga scene\n";
      return 2;
    }
    for (const auto& block : transferPlan) {
      if (!block.rigidBody) {
        std::cerr << "FAIL: transfer plan contains missing rigid body\n";
        return 2;
      }
      block.rigidBody->wake();
      block.rigidBody->setVelocity(glm::vec3(0.0f));
      block.rigidBody->setAngularVelocity(glm::vec3(0.0f));
      block.rigidBody->clearAccumulatedForce();
    }
  }

  std::cout
      << "scene=" << scenePath
      << " bodies=" << rigidBodies.size()
      << " dynamic=" << dynamicBodyCount(rigidBodies)
      << " iterations=" << solver.solverIterations
      << " substeps=" << solver.rigidSubsteps
      << " dt=" << tuning.physicsDt
      << " wake_step=" << wakeStep
      << " wake_body=" << wakeBodyName
      << "\n";

  auto constraints = std::vector<std::unique_ptr<physics::Constraint>>();

  for (int step = 0; step <= steps; ++step) {
    OverlapStats overlapStats;
    PlanePenetrationStats planeStats;
    float maxSpeed = 0.0f;
    collectDiagnostics(scene, overlapStats, planeStats, maxSpeed);

    if (testStackStability && step >= kStackTestSettleAfterStep) {
      maxSettledBoxOverlap = std::max(maxSettledBoxOverlap, overlapStats.maxDepth);
    }
    if (testPiecewiseTransfer) {
      transferProgress.peakSpeed = std::max(transferProgress.peakSpeed, maxSpeed);
      transferProgress.peakOverlap = std::max(transferProgress.peakOverlap, overlapStats.maxDepth);
      transferProgress.peakPlanePenetration =
          std::max(transferProgress.peakPlanePenetration, planeStats.maxDepth);
    }

    if (!testStackStability && !testPiecewiseTransfer &&
        (step == 0 || step <= 20 || (step % 10) == 0 || overlapStats.pairCount > 0)) {
      std::cout
          << "step=" << step
          << " max_speed=" << maxSpeed
          << " box_overlaps=" << overlapStats.pairCount
          << " max_box_overlap=" << overlapStats.maxDepth
          << " worst_pair=" << (overlapStats.bodyA.empty() ? "-" : overlapStats.bodyA + "/" + overlapStats.bodyB)
          << " plane_penetrations=" << planeStats.pairCount
          << " max_plane_penetration=" << planeStats.maxDepth
          << " worst_plane=" << (planeStats.body.empty() ? "-" : planeStats.body + "/" + planeStats.plane)
          << "\n";
    }

    if (step == dumpStep) {
      const auto dump = collectContactDump(solver, rigidBodies);
      std::cout << "contact_dump_step=" << step << " contacts=" << dump.size() << "\n";
      for (size_t i = 0; i < std::min<size_t>(dump.size(), 12); ++i) {
        const auto& entry = dump[i];
        std::cout
            << "  [" << i << "] "
            << entry.bodyA << "/" << entry.bodyB
            << " normal=(" << entry.normal.x << "," << entry.normal.y << "," << entry.normal.z << ")"
            << "\n";
      }
    }

    if (step == steps) {
      break;
    }

    if (wakeStep >= 0 && step == wakeStep) {
      for (auto* rigidBody : rigidBodies) {
        auto* owner = rigidBody ? rigidBody->getOwner() : nullptr;
        if (!owner || owner->get_name() != wakeBodyName) {
          continue;
        }

        rigidBody->wake();
        rigidBody->setVelocity(wakeVelocity);
        break;
      }
    }

    if (removeStep >= 0 && step == removeStep && !removeBodyName.empty()) {
      std::vector<std::string> removeNames;
      std::istringstream stream(removeBodyName);
      std::string token;
      while (std::getline(stream, token, ',')) {
        if (!token.empty()) removeNames.push_back(token);
      }
      for (auto* rigidBody : rigidBodies) {
        auto* owner = rigidBody ? rigidBody->getOwner() : nullptr;
        if (!owner) continue;
        const auto& name = owner->get_name();
        for (const auto& removeName : removeNames) {
          if (name == removeName) {
            rigidBody->setCollisionEnabled(false);
            rigidBody->setPosition(glm::vec3(100.0f, 100.0f, -100.0f));
            rigidBody->sleep();
            std::cout << "removed=" << name << " at step=" << step << "\n";
            break;
          }
        }
      }
    }

    sauce::physics_demo::applyForces(scene);
    if (testPiecewiseTransfer && transferProgress.activeIndex < transferPlan.size()) {
      if (transferProgress.phase < 4) {
        drivePiecewiseTransferBlock(
            solver,
            transferPlan[transferProgress.activeIndex],
            transferProgress.phase,
            tuning.physicsDt);
      } else {
        solver.dragBody = nullptr;
      }
    } else {
      solver.dragBody = nullptr;
    }
    solver.solvePositions(rigidBodies, constraints, tuning.physicsDt);
    sauce::physics_demo::syncRigidBodiesToTransforms(scene);

    if (testPiecewiseTransfer) {
      ++transferProgress.phaseSteps;
      advancePiecewiseTransferIfReady(transferProgress, transferPlan);
      if (transferProgress.completedBlocks == static_cast<int>(transferPlan.size())) {
        break;
      }
    }
  }

  if (testStackStability) {
    std::cout << "stack_stability_max_settled_box_overlap_m=" << maxSettledBoxOverlap
              << " threshold_m=" << kStackTestMaxBoxOverlapM << "\n";
    if (maxSettledBoxOverlap > kStackTestMaxBoxOverlapM) {
      std::cerr << "FAIL: dynamic box-box OBB overlap exceeds regression threshold\n";
      return 2;
    }
  }

  if (testPiecewiseTransfer) {
    float maxFinalPlacementError = 0.0f;
    for (const auto& block : transferPlan) {
      if (!block.rigidBody) {
        continue;
      }
      maxFinalPlacementError = std::max(
          maxFinalPlacementError,
          glm::distance(block.rigidBody->getPosition(), block.targetPosition));
    }

    std::cout
        << "piecewise_transfer_completed_blocks=" << transferProgress.completedBlocks
        << " active_index=" << transferProgress.activeIndex
        << " active_block="
        << ((transferProgress.activeIndex < transferPlan.size()) ? transferPlan[transferProgress.activeIndex].name : std::string("-"))
        << " phase=" << transferProgress.phase
        << " peak_speed=" << transferProgress.peakSpeed
        << " peak_box_overlap=" << transferProgress.peakOverlap
        << " peak_plane_penetration=" << transferProgress.peakPlanePenetration
        << " max_final_error=" << maxFinalPlacementError
        << "\n";

    if (transferProgress.completedBlocks != static_cast<int>(transferPlan.size())) {
      std::cerr << "FAIL: scripted transfer did not complete\n";
      return 2;
    }
    if (transferProgress.peakSpeed > kTransferTestPeakSpeedLimit) {
      std::cerr << "FAIL: scripted transfer exceeded peak speed limit\n";
      return 2;
    }
    if (transferProgress.peakOverlap > kTransferTestPeakOverlapLimitM) {
      std::cerr << "FAIL: scripted transfer exceeded peak overlap limit\n";
      return 2;
    }
    if (transferProgress.peakPlanePenetration > kTransferTestPeakPlanePenetrationLimitM) {
      std::cerr << "FAIL: scripted transfer exceeded plane penetration limit\n";
      return 2;
    }
    if (maxFinalPlacementError > kTransferTestFinalPlacementToleranceM) {
      std::cerr << "FAIL: scripted transfer final placement error is too large\n";
      return 2;
    }
  }

  return 0;
}
