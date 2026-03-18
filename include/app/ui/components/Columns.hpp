#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    /**
 * Columns - Wraps ImGui::Columns() / ImGui::BeginColumns() / ImGui::EndColumns()
 * Creates a multi-column layout
 */
    class Columns : public ImGuiComponent {
      public:
        using ContentCallback = std::function<void()>;

        Columns(const std::string& name, int count = 1, const std::string& id = "",
                bool border = true, ContentCallback content = nullptr);

        void render() override;

        void setCount(int newCount) {
            count = newCount;
        }
        int getCount() const {
            return count;
        }

        void setId(const std::string& newId) {
            id = newId;
        }
        const std::string& getId() const {
            return id;
        }

        void setBorder(bool newBorder) {
            border = newBorder;
        }
        bool hasBorder() const {
            return border;
        }

        void setContent(ContentCallback newContent) {
            content = newContent;
        }

        // Static helper to advance to next column
        static void nextColumn() {
            ImGui::NextColumn();
        }

      private:
        int count;
        std::string id;
        bool border;
        ContentCallback content;
    };

} // namespace sauce::ui
