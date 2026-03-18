#pragma once

#include <app/Entity.hpp>

namespace sauce {
    class Scene;
}

namespace sauce::editor {

    class SelectionManager {
      public:
        void select(int index) {
            selectedIndex = index;
        }
        void deselect() {
            selectedIndex = -1;
        }
        int getSelectedIndex() const {
            return selectedIndex;
        }
        bool hasSelection() const {
            return selectedIndex >= 0;
        }

        sauce::Entity* getSelectedEntity(sauce::Scene& scene);

      private:
        int selectedIndex = -1;
    };

} // namespace sauce::editor
