#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    /**
 * ChildWindow - Wraps ImGui::BeginChild() / ImGui::EndChild()
 * Creates a scrollable region within a parent window
 */
    class ChildWindow : public ImGuiComponent {
      public:
        using ContentCallback = std::function<void()>;

        ChildWindow(const std::string& name, const ImVec2& size = ImVec2(0, 0),
                    ImGuiChildFlags childFlags = 0, ImGuiWindowFlags windowFlags = 0,
                    ContentCallback content = nullptr);

        void render() override;

        void setSize(const ImVec2& newSize) {
            size = newSize;
        }
        ImVec2 getSize() const {
            return size;
        }

        void setContent(ContentCallback newContent) {
            content = newContent;
        }
        void setChildFlags(ImGuiChildFlags newFlags) {
            childFlags = newFlags;
        }
        void setWindowFlags(ImGuiWindowFlags newFlags) {
            windowFlags = newFlags;
        }

      private:
        ImVec2 size;
        ImGuiChildFlags childFlags;
        ImGuiWindowFlags windowFlags;
        ContentCallback content;
    };

} // namespace sauce::ui
