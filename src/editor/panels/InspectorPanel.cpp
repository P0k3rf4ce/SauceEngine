#include <editor/panels/InspectorPanel.hpp>
#include <editor/EditorApp.hpp>
#include <app/Scene.hpp>
#include <app/Entity.hpp>
#include <app/components/ClothComponent.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/modeling/Mesh.hpp>
#include <app/modeling/Material.hpp>
#include <app/modeling/PropertyValue.hpp>

#include <app/modeling/GLTFLoader.hpp>
#include <app/modeling/Model.hpp>

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <variant>
#include <algorithm>
#include <cstring>
#include <filesystem>

namespace sauce::editor {

namespace {

ClothComponent* findClothComponentForMesh(
    sauce::Entity& entity,
    const std::shared_ptr<sauce::modeling::Mesh>& mesh) {
  if (!mesh) {
    return nullptr;
  }

  for (auto* cloth : entity.getComponents<ClothComponent>()) {
    if (cloth->getSourceMesh() == mesh) {
      return cloth;
    }
  }

  return nullptr;
}

} // namespace


InspectorPanel::InspectorPanel(EditorApp& app)
  : EditorPanel("Inspector", app) {}

void InspectorPanel::render() {
  ImGui::Begin(title.c_str(), &isOpen);

  auto& selection = app.getSelectionManager();
  auto* entity = selection.getSelectedEntity(app.getScene());

  if (!entity) {
    ImGui::TextDisabled("No entity selected");
    ImGui::End();
    return;
  }

  // Editable entity name
  {
    char nameBuf[256];
    std::string currentName = entity->get_name();
    strncpy(nameBuf, currentName.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';

    ImGui::SetNextItemWidth(-60);
    if (ImGui::InputText("##EntityName", nameBuf, sizeof(nameBuf),
                          ImGuiInputTextFlags_EnterReturnsTrue)) {
      entity->set_name(nameBuf);
      app.setStatusMessage("Renamed to: " + std::string(nameBuf));
    }
    ImGui::SameLine();

    // Active checkbox
    bool active = entity->getActive();
    if (ImGui::Checkbox("##Active", &active)) {
      entity->setActive(active);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Active");
    }
  }

  ImGui::Separator();

  drawTransformSection(*entity);
  drawMeshRendererSection(*entity);
  drawClothSection(*entity);

  ImGui::Spacing();
  ImGui::Spacing();

  // Add Component button
  float width = ImGui::GetContentRegionAvail().x;
  if (ImGui::Button("Add Component", ImVec2(width, 0))) {
    ImGui::OpenPopup("AddComponentPopup");
  }

  if (ImGui::BeginPopup("AddComponentPopup")) {
    if (ImGui::MenuItem("Transform")) {
      if (!entity->getComponent<TransformComponent>()) {
        entity->addComponent<TransformComponent>();
        app.setStatusMessage("Added TransformComponent");
      } else {
        app.setStatusMessage("Entity already has a TransformComponent");
      }
    }
    if (ImGui::MenuItem("Mesh Renderer")) {
      entity->addComponent<MeshRendererComponent>();
      app.setStatusMessage("Added MeshRendererComponent");
    }
    if (ImGui::MenuItem("Cloth")) {
      auto* meshRenderer = entity->getComponent<MeshRendererComponent>();
      if (meshRenderer && meshRenderer->getMesh()) {
        entity->addComponent<ClothComponent>(meshRenderer->getMesh());
        auto* cloth = entity->getComponent<ClothComponent>();
        if (cloth && cloth->hasClothData()) {
          app.setStatusMessage("Added ClothComponent from mesh");
        } else if (cloth) {
          app.setStatusMessage(cloth->getLastBuildError());
        }
      } else {
        entity->addComponent<ClothComponent>();
        app.setStatusMessage("Added ClothComponent");
      }
    }
    ImGui::EndPopup();
  }

  ImGui::End();
}

static bool drawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float speed) {
  bool modified = false;

  ImGui::PushID(label.c_str());

  float labelWidth = 70.0f;
  float totalWidth = ImGui::GetContentRegionAvail().x;
  float fieldWidth = (totalWidth - labelWidth) / 3.0f - 4.0f;

  ImGui::AlignTextToFramePadding();
  ImGui::Text("%s", label.c_str());
  ImGui::SameLine(labelWidth);

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));

  const ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_AutoSelectAll;

  // X
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.35f, 0.15f, 0.15f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.45f, 0.20f, 0.20f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.55f, 0.25f, 0.25f, 1.0f));
  ImGui::SetNextItemWidth(fieldWidth);
  if (ImGui::InputFloat("##X", &values.x, 0.0f, 0.0f, "%.2f", inputFlags)) modified = true;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();

  // Y
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.35f, 0.15f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.45f, 0.20f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.55f, 0.25f, 1.0f));
  ImGui::SetNextItemWidth(fieldWidth);
  if (ImGui::InputFloat("##Y", &values.y, 0.0f, 0.0f, "%.2f", inputFlags)) modified = true;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();

  // Z
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.35f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.20f, 0.45f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.55f, 1.0f));
  ImGui::SetNextItemWidth(fieldWidth);
  if (ImGui::InputFloat("##Z", &values.z, 0.0f, 0.0f, "%.2f", inputFlags)) modified = true;
  ImGui::PopStyleColor(3);

  ImGui::PopStyleVar();

  // Normalize negative zero to positive zero
  if (values.x == 0.0f) values.x = 0.0f;
  if (values.y == 0.0f) values.y = 0.0f;
  if (values.z == 0.0f) values.z = 0.0f;

  ImGui::PopID();

  return modified;
}

void InspectorPanel::drawTransformSection(sauce::Entity& entity) {
  auto* tc = entity.getComponent<TransformComponent>();
  if (!tc) return;

  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto& transform = tc->getTransform();

    // Position
    glm::vec3 pos = transform.getTranslation();
    if (drawVec3Control("Position", pos, 0.0f, 0.1f)) {
      transform.setTranslation(pos);
    }

    // Rotation (euler degrees from quaternion)
    glm::quat rot = transform.getRotation();
    glm::vec3 euler = glm::degrees(glm::eulerAngles(rot));
    if (drawVec3Control("Rotation", euler, 0.0f, 0.5f)) {
      transform.setRotation(glm::quat(glm::radians(euler)));
    }

    // Scale
    glm::vec3 scale = transform.getScale();
    if (drawVec3Control("Scale", scale, 1.0f, 0.01f)) {
      transform.setScale(scale);
    }
  }
}

void InspectorPanel::drawMeshRendererSection(sauce::Entity& entity) {
  auto mrcs = entity.getComponents<MeshRendererComponent>();
  if (mrcs.empty()) return;

  for (size_t i = 0; i < mrcs.size(); ++i) {
    auto* mrc = mrcs[i];
    ImGui::PushID(static_cast<int>(i));

    std::string headerLabel = (mrcs.size() == 1)
      ? "Mesh Renderer"
      : "Mesh " + std::to_string(i);

    bool removeRequested = false;
    if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      auto mesh = mrc->getMesh();
      auto material = mrc->getMaterial();

      const std::string& modelPath = mrc->getModelPath();
      std::string displayName = modelPath.empty()
        ? "None"
        : std::filesystem::path(modelPath).filename().string();

      float clearBtnWidth = 24.0f;
      float labelWidth = 50.0f;
      float slotWidth = ImGui::GetContentRegionAvail().x - clearBtnWidth - ImGui::GetStyle().ItemSpacing.x - labelWidth;

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Model");
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
      ImGui::Button(displayName.c_str(), ImVec2(slotWidth, 0));
      ImGui::PopStyleColor(2);

      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Drag a .gltf/.glb from the Asset Browser to assign or replace");

      if (ImGui::BeginDragDropTarget()) {
        if (auto* payload = ImGui::AcceptDragDropPayload("ASSET_FILE")) {
          std::string path(static_cast<const char*>(payload->Data));
          std::string ext = std::filesystem::path(path).extension().string();
          if (ext == ".gltf" || ext == ".glb") {
            app.replaceModelOnComponent(*mrc, path);
          }
        }
        ImGui::EndDragDropTarget();
      }

      ImGui::SameLine();
      if (ImGui::Button("x##clearMesh", ImVec2(clearBtnWidth, 0))) {
        app.clearModelOnComponent(*mrc);
      }
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Clear mesh");

      if (mesh) {
        ImGui::Text("Vertices: %zu", mesh->getVertexCount());
        ImGui::SameLine(0, 20);
        ImGui::Text("Indices: %zu", mesh->getIndexCount());

        ClothComponent* cloth = findClothComponentForMesh(entity, mesh);
        const char* clothActionLabel =
            cloth ? "Rebuild Cloth Data" : "Create Cloth Data";
        if (ImGui::Button(clothActionLabel, ImVec2(-1.0f, 0.0f))) {
          if (!cloth) {
            entity.addComponent<ClothComponent>(mesh);
            cloth = entity.getComponent<ClothComponent>();
          } else {
            cloth->rebuildFromMesh(mesh);
          }

          if (cloth && cloth->hasClothData()) {
            app.setStatusMessage("Built cloth data from mesh");
          } else if (cloth) {
            app.setStatusMessage(cloth->getLastBuildError());
          }
        }

        const auto& meshMeta = mesh->getMetadata();
        if (!meshMeta.empty())
          drawMetadataSection("Mesh Metadata", meshMeta);
      }

      ImGui::Spacing();

      if (material) {
        ImGui::Text("Material: %s", material->getName().c_str());
        auto& props = material->getProperties();

        glm::vec4 color = props.baseColorFactor;
        if (ImGui::ColorEdit4("Base Color", glm::value_ptr(color), ImGuiColorEditFlags_Float)) {
          props.baseColorFactor = color;
        }

        if (ImGui::SliderFloat("Metallic", &props.metallicFactor, 0.0f, 1.0f, "%.2f")) {}
        if (ImGui::SliderFloat("Roughness", &props.roughnessFactor, 0.0f, 1.0f, "%.2f")) {}

        if (ImGui::TreeNode("Emissive")) {
          glm::vec3 emissive = props.emissiveFactor;
          if (ImGui::ColorEdit3("Emissive", glm::value_ptr(emissive))) {
            props.emissiveFactor = emissive;
          }
          ImGui::TreePop();
        }

        const auto& matMeta = material->getMetadata();
        if (!matMeta.empty()) {
          drawMetadataSection("Material Metadata", matMeta);
        }
      } else {
        ImGui::TextDisabled("No material assigned");
      }

      ImGui::Spacing();
      float removeWidth = ImGui::GetContentRegionAvail().x;
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.25f, 0.25f, 1.0f));
      if (ImGui::Button("Remove Component", ImVec2(removeWidth, 0))) {
        removeRequested = true;
      }
      ImGui::PopStyleColor(3);
    }

    if (removeRequested) {
      entity.removeComponentByPointer(mrc);
      app.setStatusMessage("Removed MeshRendererComponent");
      ImGui::PopID();
      break;
    }

    ImGui::PopID();
  }
}

void InspectorPanel::drawClothSection(sauce::Entity& entity) {
  auto clothComponents = entity.getComponents<ClothComponent>();
  if (clothComponents.empty()) return;

  for (size_t i = 0; i < clothComponents.size(); ++i) {
    auto* cloth = clothComponents[i];
    ImGui::PushID(static_cast<int>(i));

    const std::string headerLabel = (clothComponents.size() == 1)
        ? "Cloth"
        : "Cloth " + std::to_string(i);

    bool removeRequested = false;
    if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      auto sourceMesh = cloth->getSourceMesh();
      if (sourceMesh) {
        ImGui::Text("Source Mesh: %zu vertices / %zu indices",
                    sourceMesh->getVertexCount(),
                    sourceMesh->getIndexCount());
      } else {
        ImGui::TextDisabled("No source mesh assigned");
      }

      if (sourceMesh) {
        if (ImGui::Button("Build From Source Mesh", ImVec2(-1.0f, 0.0f))) {
          if (cloth->rebuildFromSourceMesh()) {
            app.setStatusMessage("Rebuilt cloth data from source mesh");
          } else {
            app.setStatusMessage(cloth->getLastBuildError());
          }
        }
      }

      if (cloth->hasClothData()) {
        const auto* clothData = cloth->getClothData();
        ImGui::Text("Particles: %zu", cloth->getParticleCount());
        ImGui::SameLine(0, 20);
        ImGui::Text("Triangles: %zu", cloth->getTriangleCount());
        ImGui::Text("Edges: %zu", cloth->getEdgeCount());
        ImGui::SameLine(0, 20);
        ImGui::Text("Stretches: %zu", cloth->getStretchConstraintCount());
        ImGui::Text("Bends: %zu", cloth->getBendConstraintCount());
        ImGui::SameLine(0, 20);
        ImGui::Text("Pinned: %zu", clothData->pinnedParticleCount());
        ImGui::SameLine(0, 20);
        ImGui::Text("Static: %zu", clothData->staticParticleCount());

        if (ImGui::TreeNode("Particle Preview")) {
          const size_t previewCount = std::min<size_t>(clothData->particles.size(), 4);
          for (size_t particleIndex = 0; particleIndex < previewCount; ++particleIndex) {
            const auto& particle = clothData->particles[particleIndex];
            ImGui::BulletText(
                "#%zu pos=(%.2f, %.2f, %.2f) pred=(%.2f, %.2f, %.2f) invMass=%.2f pinned=%s",
                particleIndex,
                particle.position.x, particle.position.y, particle.position.z,
                particle.predictedPosition.x, particle.predictedPosition.y, particle.predictedPosition.z,
                particle.invMass,
                particle.pinned ? "true" : "false");
          }
          ImGui::TreePop();
        }

        if (ImGui::TreeNode("Edge Preview")) {
          const size_t previewCount = std::min<size_t>(clothData->topology.edges.size(), 4);
          for (size_t edgeIndex = 0; edgeIndex < previewCount; ++edgeIndex) {
            const auto& edge = clothData->topology.edges[edgeIndex];
            ImGui::BulletText(
                "#%zu [%u, %u] triangles=(%u, %u)",
                edgeIndex,
                edge.particleIndices[0],
                edge.particleIndices[1],
                edge.adjacentTriangleIndices[0],
                edge.adjacentTriangleIndices[1]);
          }
          ImGui::TreePop();
        }

        if (ImGui::TreeNode("Stretch Preview")) {
          const size_t previewCount = std::min<size_t>(clothData->stretchConstraints.size(), 4);
          for (size_t stretchIndex = 0; stretchIndex < previewCount; ++stretchIndex) {
            const auto& stretch = clothData->stretchConstraints[stretchIndex];
            ImGui::BulletText(
                "#%zu [%u, %u] restLength=%.4f",
                stretchIndex,
                stretch.particleIndices[0],
                stretch.particleIndices[1],
                stretch.restLength);
          }
          ImGui::TreePop();
        }

        if (ImGui::TreeNode("Bend Preview")) {
          const size_t previewCount = std::min<size_t>(clothData->bendConstraints.size(), 4);
          for (size_t bendIndex = 0; bendIndex < previewCount; ++bendIndex) {
            const auto& bend = clothData->bendConstraints[bendIndex];
            ImGui::BulletText(
                "#%zu edge=[%u, %u] opp=[%u, %u] restAngle=%.4f",
                bendIndex,
                bend.sharedEdgeParticleIndices[0],
                bend.sharedEdgeParticleIndices[1],
                bend.oppositeParticleIndices[0],
                bend.oppositeParticleIndices[1],
                bend.restAngle);
          }
          ImGui::TreePop();
        }
      } else if (!cloth->getLastBuildError().empty()) {
        ImGui::TextColored(
            ImVec4(0.9f, 0.4f, 0.4f, 1.0f),
            "%s",
            cloth->getLastBuildError().c_str());
      } else {
        ImGui::TextDisabled("No cloth data generated");
      }

      ImGui::Spacing();
      float removeWidth = ImGui::GetContentRegionAvail().x;
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.25f, 0.25f, 1.0f));
      if (ImGui::Button("Remove Component", ImVec2(removeWidth, 0))) {
        removeRequested = true;
      }
      ImGui::PopStyleColor(3);
    }

    if (removeRequested) {
      entity.removeComponentByPointer(cloth);
      app.setStatusMessage("Removed ClothComponent");
      ImGui::PopID();
      break;
    }

    ImGui::PopID();
  }
}

void InspectorPanel::drawMetadataSection(
    const std::string& label,
    const std::unordered_map<std::string, sauce::modeling::PropertyValue>& metadata) {

  if (ImGui::TreeNode(label.c_str())) {
    for (const auto& [key, value] : metadata) {
      std::visit([&key](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>) {
          ImGui::Text("%s: %d", key.c_str(), val);
        } else if constexpr (std::is_same_v<T, bool>) {
          ImGui::Text("%s: %s", key.c_str(), val ? "true" : "false");
        } else if constexpr (std::is_same_v<T, float>) {
          ImGui::Text("%s: %.3f", key.c_str(), val);
        } else if constexpr (std::is_same_v<T, double>) {
          ImGui::Text("%s: %.3f", key.c_str(), val);
        } else if constexpr (std::is_same_v<T, std::string>) {
          ImGui::Text("%s: %s", key.c_str(), val.c_str());
        }
      }, value);
    }
    ImGui::TreePop();
  }
}

} // namespace sauce::editor
