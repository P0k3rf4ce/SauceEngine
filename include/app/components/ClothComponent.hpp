#pragma once

#include "app/Component.hpp"
#include "app/ClothSettings.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/Transform.hpp"

#include <physics/Cloth.hpp>

#include <memory>
#include <optional>
#include <string>

namespace sauce {

class ClothComponent : public Component {
public:
  ClothComponent();
  explicit ClothComponent(std::shared_ptr<modeling::Mesh> sourceMesh);
  ClothComponent(std::shared_ptr<modeling::Mesh> sourceMesh, const ClothSettings& settings);

  void setOwner(Entity* newOwner) override;

  bool rebuildFromMesh(std::shared_ptr<modeling::Mesh> mesh, float defaultInvMass = 1.0f);
  bool rebuildFromMesh(std::shared_ptr<modeling::Mesh> mesh, const ClothSettings& settings);
  bool rebuildFromSourceMesh();
  bool rebuildFromSourceMesh(float defaultInvMass);
  bool rebuildFromSourceMesh(const ClothSettings& settings);
  bool resetSimulation();
  void clear();

  const std::shared_ptr<modeling::Mesh>& getSourceMesh() const { return sourceMesh; }
  const std::shared_ptr<modeling::Mesh>& getRuntimeMesh() const { return runtimeMesh; }
  bool hasClothData() const { return clothData.has_value(); }
  const std::string& getLastBuildError() const { return lastBuildError; }
  const ClothSettings& getSettings() const { return settings; }
  void setSettings(const ClothSettings& newSettings);

  const physics::ClothData* getClothData() const;
  physics::ClothData* getClothData();

  void syncSimulationTransform();
  void markRuntimeMeshDirty() { runtimeMeshDirty = true; }
  bool isRuntimeMeshDirty() const { return runtimeMeshDirty; }
  bool syncRuntimeMesh(bool regenerateTangents = true);

  size_t getParticleCount() const;
  size_t getTriangleCount() const;
  size_t getEdgeCount() const;
  size_t getStretchConstraintCount() const;
  size_t getBendConstraintCount() const;

  virtual ~ClothComponent() = default;

private:
  std::shared_ptr<modeling::Mesh> sourceMesh;
  std::shared_ptr<modeling::Mesh> runtimeMesh;
  std::optional<physics::ClothData> clothData;
  ClothSettings settings;
  modeling::Transform lastSimulationTransform;
  bool runtimeMeshDirty = false;
  std::string lastBuildError;
};

} // namespace sauce
