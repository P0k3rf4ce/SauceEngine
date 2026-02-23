#pragma once

#include <editor/panels/EditorPanel.hpp>
#include <app/modeling/PropertyValue.hpp>
#include <unordered_map>

namespace sauce {

class Entity;

namespace modeling {
class Mesh;
class Material;
}

} // namespace sauce

namespace sauce::editor {

class InspectorPanel : public EditorPanel {
public:
  InspectorPanel(EditorApp& app);
  void render() override;

private:
  void drawTransformSection(sauce::Entity& entity);
  void drawMeshRendererSection(sauce::Entity& entity);
  void drawMetadataSection(const std::string& label,
                           const std::unordered_map<std::string, sauce::modeling::PropertyValue>& metadata);
};

} // namespace sauce::editor
