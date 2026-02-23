#include <editor/SelectionManager.hpp>
#include <app/Scene.hpp>

namespace sauce::editor {

sauce::Entity* SelectionManager::getSelectedEntity(sauce::Scene& scene) {
  auto& entities = scene.getEntitiesMut();
  if (selectedIndex < 0 || selectedIndex >= static_cast<int>(entities.size())) {
    return nullptr;
  }
  return &entities[selectedIndex];
}

} // namespace sauce::editor
