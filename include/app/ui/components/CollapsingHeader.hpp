#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    /**
 * CollapsingHeader - Wraps ImGui::CollapsingHeader()
 * Creates a collapsible section header
 */
    class CollapsingHeader : public ImGuiComponent {
      public:
        using ContentCallback = std::function<void()>;

        CollapsingHeader(const std::string& name, const std::string& label,
                         ContentCallback content = nullptr, ImGuiTreeNodeFlags flags = 0);

        void render() override;

        void setLabel(const std::string& newLabel) {
            label = newLabel;
        }
        const std::string& getLabel() const {
            return label;
        }

        void setContent(ContentCallback newContent) {
            content = newContent;
        }
        void setFlags(ImGuiTreeNodeFlags newFlags) {
            flags = newFlags;
        }

        bool isHeaderOpen() const {
            return headerOpen;
        }

        // Optional: track visibility with close button
        bool* getVisiblePtr() {
            return &visible;
        }
        bool isVisible() const {
            return visible;
        }
        void setVisible(bool vis) {
            visible = vis;
        }

      private:
        std::string label;
        ContentCallback content;
        ImGuiTreeNodeFlags flags;
        bool headerOpen = false;
        bool visible = true;
    };

} // namespace sauce::ui
