#include <app/Entity.hpp>
#include <app/components/ClothComponent.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/modeling/Mesh.hpp>

#include <physics/Cloth.hpp>
#include <physics/XPBD.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using physics::BendConstraint;
using physics::StretchConstraint;
using physics::ClothData;
using physics::ClothParticle;
using physics::XPBDSolver;

constexpr float kPositionEpsilon = 1e-4f;
constexpr float kScalarEpsilon = 1e-4f;

bool approxEqual(float actual, float expected, float epsilon = kScalarEpsilon) {
  return std::fabs(actual - expected) <= epsilon;
}

bool approxEqual(const glm::vec3& actual, const glm::vec3& expected, float epsilon = kPositionEpsilon) {
  return glm::length(actual - expected) <= epsilon;
}

bool isFinite(const glm::vec3& value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void appendError(std::vector<std::string>& errors, const std::string& message) {
  errors.push_back(message);
}

ClothParticle makeParticle(
    const glm::vec3& position,
    float invMass = 1.0f,
    bool pinned = false,
    const glm::vec3& velocity = glm::vec3(0.0f)) {
  return ClothParticle {
      .position = position,
      .previousPosition = position,
      .predictedPosition = position,
      .velocity = velocity,
      .invMass = invMass,
      .pinned = pinned,
  };
}

sauce::Vertex makeRenderVertex(const glm::vec3& position, const glm::vec2& uv) {
  return sauce::Vertex {
      .position = position,
      .normal = glm::vec3(0.0f, 1.0f, 0.0f),
      .texCoords = uv,
      .color = glm::vec3(1.0f),
      .tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
  };
}

std::shared_ptr<sauce::modeling::Mesh> makeQuadMesh() {
  std::vector<sauce::Vertex> vertices {
      makeRenderVertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
      makeRenderVertex(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
      makeRenderVertex(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
      makeRenderVertex(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
  };
  std::vector<uint32_t> indices { 0, 1, 2, 2, 1, 3 };

  auto mesh = std::make_shared<sauce::modeling::Mesh>(vertices, indices);
  mesh->generateNormals();
  mesh->generateTangents();
  return mesh;
}

float computeBendError(const ClothData& cloth, const BendConstraint& constraint) {
  const glm::vec3& x0 = cloth.particles[constraint.oppositeParticleIndices[0]].position;
  const glm::vec3& x1 = cloth.particles[constraint.sharedEdgeParticleIndices[0]].position;
  const glm::vec3& x2 = cloth.particles[constraint.sharedEdgeParticleIndices[1]].position;
  const glm::vec3& x3 = cloth.particles[constraint.oppositeParticleIndices[1]].position;

  const glm::vec3 e1 = x1 - x0;
  const glm::vec3 e2 = x2 - x0;
  const glm::vec3 f1 = x2 - x3;
  const glm::vec3 f2 = x1 - x3;

  const glm::vec3 triangleANormal = glm::cross(e1, e2);
  const glm::vec3 triangleBNormal = glm::cross(f1, f2);

  const float lenA = glm::length(triangleANormal);
  const float lenB = glm::length(triangleBNormal);
  if (lenA <= 1e-8f || lenB <= 1e-8f) {
    return std::numeric_limits<float>::infinity();
  }

  const float currentCos = glm::dot(triangleANormal / lenA, triangleBNormal / lenB);
  return std::fabs(currentCos - std::cos(constraint.restAngle));
}

bool testSubstepsAffectIntegration(std::vector<std::string>& errors) {
  ClothData oneSubstep;
  oneSubstep.particles.push_back(makeParticle(glm::vec3(0.0f)));

  XPBDSolver oneStepSolver;
  oneStepSolver.clothSubsteps = 1;
  oneStepSolver.solverIterations = 1;
  oneStepSolver.solveCloth(oneSubstep, 1.0f, glm::vec3(0.0f, -10.0f, 0.0f));

  if (!approxEqual(oneSubstep.particles[0].position.y, -10.0f)) {
    appendError(errors, "substeps=1 integration did not match expected semi-implicit Euler position");
    return false;
  }

  ClothData fourSubsteps;
  fourSubsteps.particles.push_back(makeParticle(glm::vec3(0.0f)));

  XPBDSolver fourStepSolver;
  fourStepSolver.clothSubsteps = 4;
  fourStepSolver.solverIterations = 1;
  fourStepSolver.solveCloth(fourSubsteps, 1.0f, glm::vec3(0.0f, -10.0f, 0.0f));

  if (!approxEqual(fourSubsteps.particles[0].position.y, -6.25f)) {
    appendError(errors, "substeps=4 integration did not produce the expected substepped position");
    return false;
  }

  if (!approxEqual(fourSubsteps.particles[0].velocity.y, -10.0f)) {
    appendError(errors, "substeps=4 integration did not preserve the expected final velocity");
    return false;
  }

  return true;
}

bool testPinnedParticlesRemainFixed(std::vector<std::string>& errors) {
  ClothData cloth;
  cloth.particles.push_back(makeParticle(glm::vec3(1.0f, 2.0f, 3.0f), 0.0f, true));

  XPBDSolver solver;
  solver.clothSubsteps = 4;
  solver.solverIterations = 8;
  solver.solveCloth(cloth, 0.5f, glm::vec3(0.0f, -9.81f, 0.0f));

  const ClothParticle& particle = cloth.particles[0];
  if (!approxEqual(particle.position, glm::vec3(1.0f, 2.0f, 3.0f))) {
    appendError(errors, "pinned particle position changed during cloth solve");
    return false;
  }
  if (!approxEqual(particle.predictedPosition, glm::vec3(1.0f, 2.0f, 3.0f))) {
    appendError(errors, "pinned particle predicted position changed during cloth solve");
    return false;
  }
  if (!approxEqual(particle.velocity, glm::vec3(0.0f))) {
    appendError(errors, "pinned particle velocity was not reset to zero");
    return false;
  }

  return true;
}

bool testStretchConstraintReducesError(std::vector<std::string>& errors) {
  ClothData cloth;
  cloth.particles.push_back(makeParticle(glm::vec3(200.0f, 200.0f, 200.0f), 1.0f, false));
  cloth.particles.push_back(makeParticle(glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, false));

  cloth.stretchConstraints.emplace_back(
      0,
      1,
	  1.f);

  XPBDSolver solver;
  solver.clothSubsteps = 4;
  solver.solverIterations = 20;
  solver.solveCloth(cloth, 1.0f / 60.0f, glm::vec3(0.0f));

  if (glm::length(cloth.particles[1].position-cloth.particles[0].position) >= 1.f) {
    appendError(errors, "what da helll");
    return false;
  }
  return true;
}

bool testBendConstraintReducesError(std::vector<std::string>& errors) {
  ClothData cloth;
  cloth.particles.push_back(makeParticle(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, true));
  cloth.particles.push_back(makeParticle(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, true));
  cloth.particles.push_back(makeParticle(glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, true));
  cloth.particles.push_back(makeParticle(glm::vec3(0.0f, 1.0f, 1.0f), 1.0f, false));

  cloth.bendConstraints.emplace_back(
      0,
      1,
      2,
      0,
      3,
      0.0f,
      0,
      1,
      0.0f);

  const BendConstraint& constraint = cloth.bendConstraints.front();
  const float errorBefore = computeBendError(cloth, constraint);
  const float zBefore = cloth.particles[3].position.z;

  XPBDSolver solver;
  solver.clothSubsteps = 4;
  solver.solverIterations = 20;
  solver.solveCloth(cloth, 1.0f / 60.0f, glm::vec3(0.0f));

  const float errorAfter = computeBendError(cloth, constraint);
  const float zAfter = cloth.particles[3].position.z;

  if (!(errorAfter < errorBefore)) {
    appendError(errors, "bend projection did not reduce hinge error toward the rest angle");
    return false;
  }

  if (!(std::fabs(zAfter) < std::fabs(zBefore))) {
    appendError(errors, "bend projection did not move the free particle back toward the rest plane");
    return false;
  }

  for (const ClothParticle& particle : cloth.particles) {
    if (!isFinite(particle.position) || !isFinite(particle.velocity) ||
        !isFinite(particle.predictedPosition)) {
      appendError(errors, "bend projection produced non-finite particle state");
      return false;
    }
  }

  return true;
}

bool testClothComponentRebuildFromMesh(std::vector<std::string>& errors) {
  auto mesh = makeQuadMesh();

  sauce::ClothSettings settings;
  settings.defaultInvMass = 0.5f;
  settings.stretchCompliance = 0.02f;
  settings.bendCompliance = 0.03f;
  settings.pinnedParticleIndices = { 0, 3, 999 };

  sauce::Entity entity("ClothEntity");
  entity.addComponent<sauce::TransformComponent>(sauce::modeling::Transform(
      glm::vec3(10.0f, 2.0f, -1.0f),
      glm::angleAxis(1.57079632679f, glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::vec3(1.0f)));
  entity.addComponent<sauce::ClothComponent>(mesh, settings);

  auto* clothComponent = entity.getComponent<sauce::ClothComponent>();
  if (!clothComponent || !clothComponent->hasClothData()) {
    appendError(errors, "ClothComponent rebuild from mesh did not produce cloth data");
    return false;
  }

  if (clothComponent->getRuntimeMesh() == nullptr ||
      clothComponent->getRuntimeMesh() == mesh) {
    appendError(errors, "ClothComponent runtime mesh was not created as a distinct copy");
    return false;
  }

  const auto* clothData = clothComponent->getClothData();
  if (!clothData || clothData->particles.size() != 4) {
    appendError(errors, "ClothComponent cloth particle count did not match source mesh vertices");
    return false;
  }

  const glm::vec3 expectedWorldPosition(10.0f, 3.0f, -1.0f);
  if (!approxEqual(clothData->particles[1].position, expectedWorldPosition)) {
    appendError(errors, "ClothComponent did not initialize particle positions in world space");
    return false;
  }

  if (!clothData->particles[0].pinned || !clothData->particles[3].pinned ||
      clothData->particles[1].pinned || clothData->particles[2].pinned) {
    appendError(errors, "ClothComponent did not apply pinned particle settings correctly");
    return false;
  }

  for (const auto& stretchConstraint : clothData->stretchConstraints) {
    if (!approxEqual(stretchConstraint.compliance, settings.stretchCompliance)) {
      appendError(errors, "ClothComponent did not apply stretch compliance to generated constraints");
      return false;
    }
  }

  for (const auto& bendConstraint : clothData->bendConstraints) {
    if (!approxEqual(bendConstraint.compliance, settings.bendCompliance)) {
      appendError(errors, "ClothComponent did not apply bend compliance to generated constraints");
      return false;
    }
  }

  sauce::ClothComponent invalidCloth;
  if (invalidCloth.rebuildFromMesh(nullptr, settings) || invalidCloth.hasClothData()) {
    appendError(errors, "ClothComponent rebuild from null mesh did not fail cleanly");
    return false;
  }

  return true;
}

bool testClothComponentTransformSync(std::vector<std::string>& errors) {
  auto mesh = makeQuadMesh();

  sauce::Entity entity("TransformSyncCloth");
  entity.addComponent<sauce::TransformComponent>(sauce::modeling::Transform(
      glm::vec3(2.0f, 0.0f, 0.0f),
      glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
      glm::vec3(1.0f)));
  entity.addComponent<sauce::ClothComponent>(mesh);

  auto* clothComponent = entity.getComponent<sauce::ClothComponent>();
  auto* transformComponent = entity.getComponent<sauce::TransformComponent>();
  auto* clothData = clothComponent ? clothComponent->getClothData() : nullptr;
  if (!clothComponent || !transformComponent || !clothData || clothData->particles.size() < 2) {
    appendError(errors, "Transform sync cloth fixture failed to initialize");
    return false;
  }

  clothData->particles[1].position = glm::vec3(3.0f, 0.0f, 0.0f);
  clothData->particles[1].previousPosition = glm::vec3(4.0f, 0.0f, 0.0f);
  clothData->particles[1].predictedPosition = glm::vec3(3.0f, 1.0f, 0.0f);
  clothData->particles[1].velocity = glm::vec3(2.0f, 0.0f, 0.0f);

  transformComponent->setTranslation(glm::vec3(5.0f, 1.0f, 0.0f));
  transformComponent->setRotation(
      glm::angleAxis(1.57079632679f, glm::vec3(0.0f, 0.0f, 1.0f)));

  clothComponent->syncSimulationTransform();

  if (!approxEqual(clothData->particles[1].position, glm::vec3(5.0f, 2.0f, 0.0f)) ||
      !approxEqual(clothData->particles[1].previousPosition, glm::vec3(5.0f, 3.0f, 0.0f)) ||
      !approxEqual(clothData->particles[1].predictedPosition, glm::vec3(4.0f, 2.0f, 0.0f)) ||
      !approxEqual(clothData->particles[1].velocity, glm::vec3(0.0f, 2.0f, 0.0f))) {
    appendError(errors, "ClothComponent did not rigidly sync particle state to owner transform changes");
    return false;
  }

  const glm::vec3 positionBeforeNoOp = clothData->particles[1].position;
  const glm::vec3 previousBeforeNoOp = clothData->particles[1].previousPosition;
  const glm::vec3 predictedBeforeNoOp = clothData->particles[1].predictedPosition;
  const glm::vec3 velocityBeforeNoOp = clothData->particles[1].velocity;

  clothComponent->syncSimulationTransform();

  if (!approxEqual(clothData->particles[1].position, positionBeforeNoOp) ||
      !approxEqual(clothData->particles[1].previousPosition, previousBeforeNoOp) ||
      !approxEqual(clothData->particles[1].predictedPosition, predictedBeforeNoOp) ||
      !approxEqual(clothData->particles[1].velocity, velocityBeforeNoOp)) {
    appendError(errors, "ClothComponent repeated transform sync was not a no-op");
    return false;
  }

  sauce::Entity noTransformEntity("NoTransformCloth");
  noTransformEntity.addComponent<sauce::ClothComponent>(mesh);
  auto* noTransformCloth = noTransformEntity.getComponent<sauce::ClothComponent>();
  auto* noTransformClothData = noTransformCloth ? noTransformCloth->getClothData() : nullptr;
  if (!noTransformCloth || !noTransformClothData ||
      !approxEqual(noTransformClothData->particles[1].position, glm::vec3(1.0f, 0.0f, 0.0f))) {
    appendError(errors, "ClothComponent did not treat a missing TransformComponent as identity");
    return false;
  }

  return true;
}

bool testClothComponentRuntimeMeshSync(std::vector<std::string>& errors) {
  auto mesh = makeQuadMesh();

  sauce::Entity entity("RuntimeMeshCloth");
  entity.addComponent<sauce::TransformComponent>(sauce::modeling::Transform(
      glm::vec3(10.0f, 0.0f, 0.0f),
      glm::angleAxis(1.57079632679f, glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::vec3(1.0f)));
  entity.addComponent<sauce::ClothComponent>(mesh);

  auto* clothComponent = entity.getComponent<sauce::ClothComponent>();
  auto* clothData = clothComponent ? clothComponent->getClothData() : nullptr;
  if (!clothComponent || !clothData || clothData->particles.size() < 2) {
    appendError(errors, "Runtime mesh sync cloth fixture failed to initialize");
    return false;
  }

  clothData->particles[1].position = glm::vec3(10.0f, 2.0f, 0.0f);
  if (!clothComponent->syncRuntimeMesh()) {
    appendError(errors, "ClothComponent syncRuntimeMesh failed unexpectedly");
    return false;
  }

  const auto runtimeMesh = clothComponent->getRuntimeMesh();
  if (!runtimeMesh ||
      !approxEqual(runtimeMesh->getVertices()[1].position, glm::vec3(2.0f, 0.0f, 0.0f))) {
    appendError(errors, "ClothComponent syncRuntimeMesh did not write inverse-transformed local vertex positions");
    return false;
  }

  for (const auto& vertex : runtimeMesh->getVertices()) {
    if (!isFinite(vertex.position) || !isFinite(vertex.normal) ||
        !isFinite(glm::vec3(vertex.tangent))) {
      appendError(errors, "ClothComponent syncRuntimeMesh produced non-finite mesh attributes");
      return false;
    }
  }

  return true;
}

} // namespace

int main() {
  std::vector<std::string> errors;

  const bool substepsOk = testSubstepsAffectIntegration(errors);
  const bool pinnedOk = testPinnedParticlesRemainFixed(errors);
  const bool bendOk = testBendConstraintReducesError(errors);
  const bool stretchOk = testStretchConstraintReducesError(errors);
  const bool componentRebuildOk = testClothComponentRebuildFromMesh(errors);
  const bool componentTransformSyncOk = testClothComponentTransformSync(errors);
  const bool componentRuntimeMeshSyncOk = testClothComponentRuntimeMeshSync(errors);

  if (!errors.empty()) {
    std::cerr << "XPBD cloth harness failed:\n";
    for (const std::string& error : errors) {
      std::cerr << "  - " << error << "\n";
    }
    return 1;
  }

  std::cout << "XPBD cloth harness passed\n";
  std::cout << "  substeps: " << (substepsOk ? "ok" : "failed") << "\n";
  std::cout << "  pinned: " << (pinnedOk ? "ok" : "failed") << "\n";
  std::cout << "  bend: " << (bendOk ? "ok" : "failed") << "\n";
  std::cout << "  stretch: " << (stretchOk ? "ok" : "failed") << "\n";
  std::cout << "  component rebuild: " << (componentRebuildOk ? "ok" : "failed") << "\n";
  std::cout << "  transform sync: " << (componentTransformSyncOk ? "ok" : "failed") << "\n";
  std::cout << "  runtime mesh sync: " << (componentRuntimeMeshSyncOk ? "ok" : "failed") << "\n";
  return 0;
}
