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

bool ClothComponent::rebuildFromMesh(
    std::shared_ptr<modeling::Mesh> mesh,
    float defaultInvMass) {
  sourceMesh = std::move(mesh);
  clothData.reset();

  if (!sourceMesh) {
    lastBuildError = "No source mesh assigned.";
    return false;
  }

  auto builtCloth = physics::buildClothDataFromMesh(*sourceMesh, "Mesh", defaultInvMass);
  if (!builtCloth.has_value()) {
    lastBuildError = "Mesh must contain valid triangle indices to build cloth data.";
    return false;
  }

  clothData = std::move(builtCloth);
  lastBuildError.clear();
  return true;
}

bool ClothComponent::rebuildFromSourceMesh(float defaultInvMass) {
  return rebuildFromMesh(sourceMesh, defaultInvMass);
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
