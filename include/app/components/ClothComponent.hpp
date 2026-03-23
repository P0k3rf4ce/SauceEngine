#pragma once

#include "app/Component.hpp"
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

  bool rebuildFromMesh(std::shared_ptr<modeling::Mesh> mesh, float defaultInvMass = 1.0f);
  bool rebuildFromSourceMesh(float defaultInvMass = 1.0f);
  void clear();

  const std::shared_ptr<modeling::Mesh>& getSourceMesh() const { return sourceMesh; }
  bool hasClothData() const { return clothData.has_value(); }
  const std::string& getLastBuildError() const { return lastBuildError; }

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
  std::string lastBuildError;
};

} // namespace sauce
