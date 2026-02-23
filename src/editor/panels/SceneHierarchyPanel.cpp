#include <editor/panels/SceneHierarchyPanel.hpp>
#include <editor/EditorApp.hpp>
#include <app/Scene.hpp>
#include <app/Entity.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>

#include <imgui.h>
#include <algorithm>
#include <filesystem>

namespace sauce::editor {

SceneHierarchyPanel::SceneHierarchyPanel(EditorApp& app)
  : EditorPanel("Hierarchy", app) {}

void SceneHierarchyPanel::render() {
  ImGui::Begin(title.c_str(), &isOpen);

  auto& scene = app.getScene();
  auto& entities = scene.getEntitiesMut();
  auto& selection = app.getSelectionManager();

  // Search/filter bar
  static char searchBuf[128] = "";
  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint("##Search", "Filter entities...", searchBuf, sizeof(searchBuf));

  ImGui::Separator();

  std::string filter(searchBuf);

  for (int i = 0; i < static_cast<int>(entities.size()); ++i) {
    auto& entity = entities[i];
    std::string name = entity.get_name();

    // Filter
    if (!filter.empty()) {
      std::string lowerName = name;
      std::string lowerFilter = filter;
      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
      std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
      if (lowerName.find(lowerFilter) == std::string::npos) {
        continue;
      }
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                               ImGuiTreeNodeFlags_NoTreePushOnOpen |
                               ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selection.getSelectedIndex() == i) {
      flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool active = entity.getActive();
    if (!active) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    }

    // Show component indicators
    bool hasMesh = entity.getComponent<MeshRendererComponent>() != nullptr;
    const char* icon = hasMesh ? "[M] " : "    ";
    char label[256];
    snprintf(label, sizeof(label), "%s%s", icon, name.c_str());

    ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(i)), flags, "%s", label);

    if (!active) {
      ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked()) {
      selection.select(i);
    }

    // Double-click to focus
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
      auto* tc = entity.getComponent<TransformComponent>();
      if (tc) {
        app.getEditorCamera().focusOn(tc->getTranslation());
        app.setStatusMessage("Focused on: " + name);
      }
    }

    // Right-click context menu
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Duplicate")) {
        sauce::Entity copy(name + " (Copy)");
        auto* tc = entity.getComponent<TransformComponent>();
        if (tc) {
          copy.addComponent<TransformComponent>(tc->getTransform());
        }
        scene.addEntity(std::move(copy));
        app.setStatusMessage("Duplicated: " + name);
      }
      if (ImGui::MenuItem("Delete")) {
        std::string deletedName = name;
        if (selection.getSelectedIndex() == i) {
          selection.deselect();
        } else if (selection.getSelectedIndex() > i) {
          selection.select(selection.getSelectedIndex() - 1);
        }
        entities.erase(entities.begin() + i);
        app.setStatusMessage("Deleted: " + deletedName);
        ImGui::EndPopup();
        break;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Toggle Active")) {
        entity.setActive(!entity.getActive());
      }
      ImGui::EndPopup();
    }
  }

  // Drop target for GLTF files in empty area
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_FILE")) {
      std::string path(static_cast<const char*>(payload->Data));
      std::string ext = std::filesystem::path(path).extension().string();
      if (ext == ".gltf" || ext == ".glb") {
        app.importGLTFToScene(path);
      }
    }
    ImGui::EndDragDropTarget();
  }

  // Right-click in empty space
  if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
    if (ImGui::MenuItem("Add Empty Entity")) {
      sauce::Entity newEntity("New Entity");
      newEntity.addComponent<TransformComponent>();
      scene.addEntity(std::move(newEntity));
      selection.select(static_cast<int>(entities.size()) - 1);
      app.setStatusMessage("Created new entity");
    }
    ImGui::EndPopup();
  }

  ImGui::End();
}

} // namespace sauce::editor
