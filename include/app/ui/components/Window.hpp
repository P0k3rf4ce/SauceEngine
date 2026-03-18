#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    /**
 * Window - Wraps ImGui::Begin() / ImGui::End()
 * Creates a window container that can hold other UI elements
 */
    class Window : public ImGuiComponent {
      public:
        using ContentCallback = std::function<void()>;

        Window(const std::string& name, const std::string& title, ContentCallback content = nullptr,
               ImGuiWindowFlags flags = 0);

        void render() override;

        void setTitle(const std::string& newTitle) {
            title = newTitle;
        }
        const std::string& getTitle() const {
            return title;
        }

        void setContent(ContentCallback newContent) {
            content = newContent;
        }
        void setFlags(ImGuiWindowFlags newFlags) {
            flags = newFlags;
        }

        bool* getOpenPtr() {
            return &isOpen;
        }
        bool isWindowOpen() const {
            return isOpen;
        }
        void setOpen(bool open) {
            isOpen = open;
        }

      private:
        std::string title;
        ContentCallback content;
        ImGuiWindowFlags flags;
        bool isOpen = true;
    };

} // namespace sauce::ui
