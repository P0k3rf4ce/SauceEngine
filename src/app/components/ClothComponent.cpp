#include "app/components/ClothComponent.hpp"

#include <utility>

namespace sauce {

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
  for (auto& constraint : clothData->stretchConstraints) {
    constraint.compliance = settings.stretchCompliance;
  }
  for (auto& constraint : clothData->bendConstraints) {
    constraint.compliance = settings.bendCompliance;
  }
  for (uint32_t particleIndex : settings.pinnedParticleIndices) {
    if (particleIndex < clothData->particles.size()) {
      clothData->particles[particleIndex].pinned = true;
    }
  }

  lastBuildError.clear();
  return true;
}

bool ClothComponent::rebuildFromSourceMesh(float defaultInvMass) {
  ClothSettings updatedSettings = settings;
  updatedSettings.defaultInvMass = defaultInvMass;
  return rebuildFromSourceMesh(updatedSettings);
}

bool ClothComponent::rebuildFromSourceMesh(const ClothSettings& newSettings) {
  return rebuildFromMesh(sourceMesh, newSettings);
}

void ClothComponent::clear() {
  clothData.reset();
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

} // namespace sauce
