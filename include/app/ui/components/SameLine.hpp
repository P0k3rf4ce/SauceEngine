#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    /**
 * SameLine - Wraps ImGui::SameLine()
 * Places the next element on the same line (horizontal layout)
 */
    class SameLine : public ImGuiComponent {
      public:
        explicit SameLine(const std::string& name, float offsetFromStartX = 0.0f,
                          float spacing = -1.0f);

        void render() override;

        void setOffsetFromStartX(float offset) {
            offsetFromStartX = offset;
        }
        float getOffsetFromStartX() const {
            return offsetFromStartX;
        }

        void setSpacing(float newSpacing) {
            spacing = newSpacing;
        }
        float getSpacing() const {
            return spacing;
        }

      private:
        float offsetFromStartX;
        float spacing;
    };

} // namespace sauce::ui
