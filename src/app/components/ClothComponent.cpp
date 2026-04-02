#include "app/components/ClothComponent.hpp"

#include "app/Entity.hpp"
#include "app/components/TransformComponent.hpp"

#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <utility>

namespace sauce {
namespace {

glm::quat normalizedRotation(const glm::quat& rotation) {
  const float len = glm::length(rotation);
  if (len <= 1e-8f) {
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  }

  return rotation / len;
}

modeling::Transform getSimulationTransform(Entity* owner) {
  if (!owner) {
    return {};
  }

  auto* transform = owner->getComponent<TransformComponent>();
  if (!transform) {
    return {};
  }

  return modeling::Transform(
      transform->getTranslation(),
      normalizedRotation(transform->getRotation()),
      glm::vec3(1.0f));
}

glm::vec3 toWorldPosition(
    const modeling::Transform& transform,
    const glm::vec3& localPosition) {
  return transform.getRotation() * localPosition + transform.getTranslation();
}

glm::vec3 toLocalPosition(
    const modeling::Transform& transform,
    const glm::vec3& worldPosition) {
  return glm::inverse(transform.getRotation()) *
         (worldPosition - transform.getTranslation());
}

std::shared_ptr<modeling::Mesh> cloneMeshCpuData(
    const std::shared_ptr<modeling::Mesh>& mesh) {
  if (!mesh) {
    return nullptr;
  }

  auto clone = std::make_shared<modeling::Mesh>(
      mesh->getVertices(),
      mesh->getIndices());
  for (const auto& [key, value] : mesh->getMetadata()) {
    clone->setMetadata(key, value);
  }
  return clone;
}

void applySettingsToClothData(physics::ClothData& data, const ClothSettings& settings) {
  for (auto& constraint : data.stretchConstraints) {
    constraint.compliance = settings.stretchCompliance;
  }
  for (auto& constraint : data.bendConstraints) {
    constraint.compliance = settings.bendCompliance;
  }

  for (auto& particle : data.particles) {
    particle.pinned = false;
  }
  for (uint32_t particleIndex : settings.pinnedParticleIndices) {
    if (particleIndex < data.particles.size()) {
      data.particles[particleIndex].pinned = true;
    }
  }
}

} // namespace

ClothComponent::ClothComponent()
    : Component("ClothComponent") {
}

ClothComponent::ClothComponent(std::shared_ptr<modeling::Mesh> sourceMesh)
    : Component("ClothComponent") {
  rebuildFromMesh(std::move(sourceMesh));
}

ClothComponent::ClothComponent(
    std::shared_ptr<modeling::Mesh> sourceMesh,
    const ClothSettings& settings)
    : Component("ClothComponent") {
  rebuildFromMesh(std::move(sourceMesh), settings);
}

void ClothComponent::setOwner(Entity* newOwner) {
  Component::setOwner(newOwner);
  syncSimulationTransform();
}

void ClothComponent::setSettings(const ClothSettings& newSettings) {
  settings = newSettings;
  if (clothData.has_value()) {
    applySettingsToClothData(*clothData, settings);
  }
}

bool ClothComponent::rebuildFromMesh(
    std::shared_ptr<modeling::Mesh> mesh,
    float defaultInvMass) {
  ClothSettings updatedSettings = settings;
  updatedSettings.defaultInvMass = defaultInvMass;
  return rebuildFromMesh(std::move(mesh), updatedSettings);
}

bool ClothComponent::rebuildFromMesh(
    std::shared_ptr<modeling::Mesh> mesh,
    const ClothSettings& newSettings) {
  sourceMesh = std::move(mesh);
  runtimeMesh.reset();
  clothData.reset();
  settings = newSettings;

  if (!sourceMesh) {
    lastBuildError = "No source mesh assigned.";
    return false;
  }

  auto builtCloth = physics::buildClothDataFromMesh(
      *sourceMesh,
      "Mesh",
      settings.defaultInvMass);
  if (!builtCloth.has_value()) {
    lastBuildError = "Mesh must contain valid triangle indices to build cloth data.";
    return false;
  }

  clothData = std::move(builtCloth);
  applySettingsToClothData(*clothData, settings);

  runtimeMesh = cloneMeshCpuData(sourceMesh);
  lastSimulationTransform = getSimulationTransform(getOwner());

  for (auto& particle : clothData->particles) {
    const glm::vec3 worldPosition =
        toWorldPosition(lastSimulationTransform, particle.position);
    particle.position = worldPosition;
    particle.previousPosition = worldPosition;
    particle.predictedPosition = worldPosition;
  }

  if (!syncRuntimeMesh()) {
    return false;
  }

  lastBuildError.clear();
  return true;
}

bool ClothComponent::rebuildFromSourceMesh() {
  return rebuildFromSourceMesh(settings);
}

bool ClothComponent::rebuildFromSourceMesh(float defaultInvMass) {
  ClothSettings updatedSettings = settings;
  updatedSettings.defaultInvMass = defaultInvMass;
  return rebuildFromSourceMesh(updatedSettings);
}

bool ClothComponent::rebuildFromSourceMesh(const ClothSettings& newSettings) {
  return rebuildFromMesh(sourceMesh, newSettings);
}

bool ClothComponent::resetSimulation() {
  return rebuildFromSourceMesh(settings);
}

void ClothComponent::clear() {
  runtimeMesh.reset();
  clothData.reset();
  lastSimulationTransform = {};
  lastBuildError.clear();
}

const physics::ClothData* ClothComponent::getClothData() const {
  return clothData ? &clothData.value() : nullptr;
}

physics::ClothData* ClothComponent::getClothData() {
  return clothData ? &clothData.value() : nullptr;
}

size_t ClothComponent::getParticleCount() const {
  const auto* data = getClothData();
  return data ? data->particles.size() : 0;
}

size_t ClothComponent::getTriangleCount() const {
  const auto* data = getClothData();
  return data ? data->topology.triangleCount() : 0;
}

size_t ClothComponent::getEdgeCount() const {
  const auto* data = getClothData();
  return data ? data->topology.edges.size() : 0;
}

size_t ClothComponent::getStretchConstraintCount() const {
  const auto* data = getClothData();
  return data ? data->stretchConstraints.size() : 0;
}

size_t ClothComponent::getBendConstraintCount() const {
  const auto* data = getClothData();
  return data ? data->bendConstraints.size() : 0;
}

void ClothComponent::syncSimulationTransform() {
  const modeling::Transform currentTransform = getSimulationTransform(getOwner());
  if (!clothData.has_value()) {
    lastSimulationTransform = currentTransform;
    return;
  }

  const glm::quat previousRotation =
      normalizedRotation(lastSimulationTransform.getRotation());
  const glm::quat currentRotation =
      normalizedRotation(currentTransform.getRotation());
  const glm::quat deltaRotation =
      normalizedRotation(currentRotation * glm::inverse(previousRotation));
  const glm::vec3 deltaTranslation =
      currentTransform.getTranslation() -
      deltaRotation * lastSimulationTransform.getTranslation();

  for (auto& particle : clothData->particles) {
    particle.position = deltaRotation * particle.position + deltaTranslation;
    particle.previousPosition =
        deltaRotation * particle.previousPosition + deltaTranslation;
    particle.predictedPosition =
        deltaRotation * particle.predictedPosition + deltaTranslation;
    particle.velocity = deltaRotation * particle.velocity;
  }

  lastSimulationTransform = currentTransform;
}

bool ClothComponent::syncRuntimeMesh() {
  if (!clothData.has_value() || !runtimeMesh) {
    lastBuildError = "No cloth data or runtime mesh available.";
    return false;
  }

  auto& vertices = runtimeMesh->getVerticesMutable();
  if (vertices.size() != clothData->particles.size()) {
    lastBuildError = "Runtime mesh vertex count does not match cloth particle count.";
    return false;
  }

  const modeling::Transform currentTransform = getSimulationTransform(getOwner());
  for (size_t i = 0; i < clothData->particles.size(); ++i) {
    vertices[i].position = toLocalPosition(currentTransform, clothData->particles[i].position);
  }

  runtimeMesh->generateNormals();
  runtimeMesh->generateTangents();
  lastBuildError.clear();
  return true;
}

} // namespace sauce
