#include <physics/Cloth.hpp>
#include <physics/XPBD.hpp>

#include <glm/glm.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using physics::BendConstraint;
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

} // namespace

int main() {
  std::vector<std::string> errors;

  const bool substepsOk = testSubstepsAffectIntegration(errors);
  const bool pinnedOk = testPinnedParticlesRemainFixed(errors);
  const bool bendOk = testBendConstraintReducesError(errors);

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
  return 0;
}
