#pragma once

#include <editor/panels/EditorPanel.hpp>
#include <imgui.h>

namespace sauce::editor {

    class ViewportPanel : public EditorPanel {
      public:
        ViewportPanel(EditorApp& app);
        void render() override;

        bool isViewportHovered() const {
            return hovered;
        }
        bool isViewportFocused() const {
            return focused;
        }

        ImVec2 getViewportSize() const {
            return viewportSize;
        }
        ImVec2 getViewportScreenPos() const {
            return viewportScreenPos;
        }
        bool viewportSizeChanged() const {
            return sizeChanged;
        }
        void clearSizeChanged() {
            sizeChanged = false;
        }

      private:
        bool hovered = false;
        bool focused = false;
        ImVec2 viewportSize = {0, 0};
        ImVec2 viewportScreenPos = {0, 0};
        bool sizeChanged = false;
    };

} // namespace sauce::editor
