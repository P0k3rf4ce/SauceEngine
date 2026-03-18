#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    /**
 * Group - Wraps ImGui::BeginGroup() / ImGui::EndGroup()
 * Groups widgets together (useful for alignment, drag and drop, etc.)
 */
    class Group : public ImGuiComponent {
      public:
        using ContentCallback = std::function<void()>;

        Group(const std::string& name, ContentCallback content = nullptr);

        void render() override;

        void setContent(ContentCallback newContent) {
            content = newContent;
        }

      private:
        ContentCallback content;
    };

} // namespace sauce::ui
