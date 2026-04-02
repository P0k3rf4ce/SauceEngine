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
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
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
  std::string scenePath = "testScene/jenga_tower.gltf";
  int steps = 180;
  int dumpStep = -1;
  int wakeStep = -1;
  std::string wakeBodyName = "JengaBlock_0";
  float wakeSpeed = 1.2f;
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
  std::string removeBodyName;
  int removeStep = -1;
  if (argc >= 8) {
    removeBodyName = argv[7];
  }
  if (argc >= 9) {
    removeStep = std::atoi(argv[8]);
  }
  const glm::vec3 wakeVelocity(wakeSpeed, 0.0f, 0.0f);

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

    if (step == 0 || step <= 20 || (step % 10) == 0 || overlapStats.pairCount > 0) {
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
    solver.solvePositions(rigidBodies, constraints, tuning.physicsDt);
    sauce::physics_demo::syncRigidBodiesToTransforms(scene);
  }

  return 0;
}
