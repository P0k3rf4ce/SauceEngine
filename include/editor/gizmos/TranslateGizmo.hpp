#pragma once

#include <editor/gizmos/Gizmo.hpp>

namespace sauce::editor {

class TranslateGizmo : public Gizmo {
public:
  GizmoType getType() const override { return GizmoType::Translate; }
  GizmoMeshData generateMesh() const override;
  GizmoAxis hitTest(const Ray& ray, const glm::vec3& position, float scale) const override;
  void beginInteraction(GizmoAxis axis, const Ray& ray, const glm::vec3& entityPos) override;
  glm::vec3 updateInteraction(const Ray& ray, const glm::vec3& entityPos) override;
  void endInteraction() override;

private:
  float initialT = 0.0f;
  glm::vec3 lastEntityPos{0.0f};

  static constexpr float SHAFT_LENGTH = 1.0f;
  static constexpr float SHAFT_RADIUS = 0.02f;
  static constexpr float CONE_LENGTH = 0.2f;
  static constexpr float CONE_RADIUS = 0.06f;
  static constexpr int SEGMENTS = 12;
};

} // namespace sauce::editor
