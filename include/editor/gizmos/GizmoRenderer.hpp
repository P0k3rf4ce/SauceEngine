#pragma once

#include <editor/EditorCamera.hpp>
#include <editor/gizmos/Gizmo.hpp>
#include <editor/gizmos/RotateGizmo.hpp>
#include <editor/gizmos/ScaleGizmo.hpp>
#include <editor/gizmos/TranslateGizmo.hpp>

#include <app/GraphicsPipeline.hpp>
#include <app/LogicalDevice.hpp>
#include <app/PhysicalDevice.hpp>
#include <app/Renderer.hpp>
#include <app/modeling/Mesh.hpp>

#include <memory>

namespace sauce::editor {

    class GizmoRenderer {
      public:
        GizmoRenderer(const sauce::PhysicalDevice& physicalDevice,
                      const sauce::LogicalDevice& logicalDevice,
                      const vk::raii::DescriptorSetLayout& descriptorSetLayout,
                      vk::Format colorFormat, sauce::Renderer& renderer);

        void setActiveGizmo(GizmoType type);
        GizmoType getActiveGizmoType() const {
            return activeType;
        }
        Gizmo* getActiveGizmo() {
            return activeGizmo.get();
        }

        void render(vk::raii::CommandBuffer& cmd, const vk::raii::DescriptorSet& descriptorSet,
                    const glm::vec3& entityPosition, const EditorCamera& camera, float aspect);

        static constexpr float SCALE_FACTOR = 0.15f;

      private:
        void uploadMesh();

        const sauce::PhysicalDevice* pPhysicalDevice;
        const sauce::LogicalDevice* pLogicalDevice;
        sauce::Renderer* pRenderer;

        std::unique_ptr<sauce::GraphicsPipeline> pipeline;
        std::unique_ptr<sauce::modeling::Mesh> mesh;

        GizmoType activeType = GizmoType::Translate;
        std::unique_ptr<Gizmo> activeGizmo;

        bool meshDirty = true;
    };

} // namespace sauce::editor
