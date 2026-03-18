#pragma once

#include <editor/gizmos/Gizmo.hpp>

namespace sauce::editor {

    class RotateGizmo : public Gizmo {
      public:
        GizmoType getType() const override {
            return GizmoType::Rotate;
        }
        GizmoMeshData generateMesh() const override;
        GizmoAxis hitTest(const Ray& ray, const glm::vec3& position, const glm::quat& rotation,
                          float scale) const override;
        void beginInteraction(GizmoAxis axis, const Ray& ray, const glm::vec3& entityPos,
                              const glm::quat& rotation) override;
        glm::vec3 updateInteraction(const Ray& ray, const glm::vec3& entityPos,
                                    const glm::quat& rotation) override;
        void endInteraction() override;

      private:
        float lastAngle = 0.0f;
        glm::vec3 lastEntityPos{0.0f};

        static constexpr float RING_RADIUS = 1.0f;
        static constexpr float TUBE_RADIUS = 0.02f;
        static constexpr int RING_SEGMENTS = 48;
        static constexpr int TUBE_SEGMENTS = 8;

        float angleOnPlane(const Ray& ray, const glm::vec3& center, const glm::vec3& normal) const;
    };

} // namespace sauce::editor
