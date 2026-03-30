#pragma once

#include "app/Component.hpp"
#include "app/ClothSettings.hpp"
#include "app/modeling/Mesh.hpp"

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

  bool rebuildFromMesh(std::shared_ptr<modeling::Mesh> mesh, float defaultInvMass = 1.0f);
  bool rebuildFromMesh(std::shared_ptr<modeling::Mesh> mesh, const ClothSettings& settings);
  bool rebuildFromSourceMesh(float defaultInvMass = 1.0f);
  bool rebuildFromSourceMesh(const ClothSettings& settings);
  void clear();

  const std::shared_ptr<modeling::Mesh>& getSourceMesh() const { return sourceMesh; }
  bool hasClothData() const { return clothData.has_value(); }
  const std::string& getLastBuildError() const { return lastBuildError; }
  const ClothSettings& getSettings() const { return settings; }
  void setSettings(const ClothSettings& newSettings) { settings = newSettings; }

  const physics::ClothData* getClothData() const;
  physics::ClothData* getClothData();

  size_t getParticleCount() const;
  size_t getTriangleCount() const;
  size_t getEdgeCount() const;
  size_t getStretchConstraintCount() const;
  size_t getBendConstraintCount() const;

  virtual ~ClothComponent() = default;

private:
  std::shared_ptr<modeling::Mesh> sourceMesh;
  std::optional<physics::ClothData> clothData;
  ClothSettings settings;
  std::string lastBuildError;
};

} // namespace sauce
