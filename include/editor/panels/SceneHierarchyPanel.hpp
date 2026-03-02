#pragma once

#include <editor/panels/EditorPanel.hpp>

namespace sauce::editor {

class SceneHierarchyPanel : public EditorPanel {
public:
  SceneHierarchyPanel(EditorApp& app);
  void render() override;
};

} // namespace sauce::editor
